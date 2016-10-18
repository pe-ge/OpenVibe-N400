#include "ovpCBoxN400Experiment.h"

#include "boost/filesystem.hpp"
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::N400;

using namespace boost::filesystem;

namespace OpenViBEPlugins
{
	namespace N400
	{
		// Called when a key is pressed on the keyboard
		gboolean N400Experiment_KeyPressCallback(GtkWidget *widget, GdkEventKey *thisEvent, gpointer data)
		{
			//std::cout << "registering key callback" << std::endl;
			reinterpret_cast<CN400Experiment*>(data)->processKey(thisEvent->keyval);
			return true;
		}

		OpenViBE::boolean filenamesCompare(const std::pair<OpenViBE::CString, ::GdkPixbuf*>& firstElem, std::pair<OpenViBE::CString, ::GdkPixbuf*>& secondElem)
		{
			std::string firstPath = firstElem.first.toASCIIString();
			std::string secondPath = secondElem.first.toASCIIString();

			std::string firstFilename = firstPath.substr(firstPath.find("\\") + 1);
			std::string secondFilename = secondPath.substr(secondPath.find("\\") + 1);

			int firstId = std::stoi(firstFilename.substr(0, firstFilename.find("_")));
			int secondId = std::stoi(secondFilename.substr(0, secondFilename.find("_")));

			return firstId < secondId;
		}

		CN400Experiment::CN400Experiment(void) :
			m_ui64CrossDuration(0),
			m_ui64PictureDuration(0),
			m_ui64PauseDuration(0),
			m_pBuilderInterface(NULL),
			m_pMainWindow(NULL),
			m_pDrawingArea(NULL),
			m_ui32RequestedPictureID(1),
			m_bRequestDraw(false),
			m_bNewIteration(true),
			m_eCurrentCue(CROSS),
			m_ui64PreviousActivationTime(0),
			m_ui64NewIterationTime(0),
			m_sRightButton(NULL),
			m_sWrongButton(NULL),
			m_sUnsureButton(NULL),
			m_bProcessingKeys(false),
			m_ui32PressedButton(0),
			m_bRequestProcessButton(false),
			m_bRequestBeep(false),
			m_bExperimentStarted(false)
		{
			m_oBackgroundColor.pixel = 0;
			m_oBackgroundColor.red = 0xFFFF;
			m_oBackgroundColor.green = 0xFFFF;
			m_oBackgroundColor.blue = 0xFFFF;

			m_oForegroundColor.pixel = 0;
			m_oForegroundColor.red = 0xFFFF;
			m_oForegroundColor.green = 0xFFFF;
			m_oForegroundColor.blue = 0xFFFF;
		}

		OpenViBE::boolean CN400Experiment::initialize()
		{
			//>>>> Reading Settings:

			// Window size
			OpenViBE::uint32 l_ui32WindowWidth		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			OpenViBE::uint32 l_ui32WindowHeight		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			// Durations
			m_ui64CrossDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			m_ui64PictureDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			m_ui64PauseDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

			// Button codes
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(5, m_sRightButton);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(6, m_sWrongButton);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(7, m_sUnsureButton);

			// Get experiment directory
			std::string l_sExperimentDirectory = OpenViBE::Directories::getDataDir() + "/../../../n400/experiment";
			OpenViBE::uint64 experimentCount = 1;
			while (boost::filesystem::exists(l_sExperimentDirectory + std::to_string(experimentCount)))
			{
				experimentCount++;
			}
			experimentCount--;
			l_sExperimentDirectory += std::to_string(experimentCount);

			// Load all files inside directory
			path p(l_sExperimentDirectory);
			directory_iterator end_itr;

			for (directory_iterator itr(p); itr != end_itr; ++itr)
			{
				if (is_regular_file(itr->path()))
				{
					CString filename(itr->path().string().c_str());
					::GdkPixbuf* l_pOriginalPicture = gdk_pixbuf_new_from_file_at_size(filename, -1, -1, NULL);
					::GdkPixbuf* l_pScaledPicture = gdk_pixbuf_scale_simple(l_pOriginalPicture, l_ui32WindowWidth, l_ui32WindowHeight, GDK_INTERP_BILINEAR);
					if (l_pOriginalPicture)
					{
						m_pOriginalPicture.push_back(std::make_pair(filename, l_pOriginalPicture));
						m_pScaledPicture.push_back(std::make_pair(filename, l_pScaledPicture));
					}
					else
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Error couldn't load ressource file : " << filename << "!\n";
						return false;
					}
				}
			}

			if (m_pOriginalPicture.empty()){
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error, no images in directory!";
				return false;
			}

			// Sort files according to digits in beginning of filename
			std::sort(m_pOriginalPicture.begin(), m_pOriginalPicture.end(), filenamesCompare);
			std::sort(m_pScaledPicture.begin(), m_pScaledPicture.end(), filenamesCompare);

			/* std::cout << "LOADING FILES" << std::endl;
			for (std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>>::const_iterator it = m_pOriginalPicture.begin(); it != m_pOriginalPicture.end(); it++)
			{
				std::cout << it->first.toASCIIString() << std::endl;
			} */

			//load the gtk builder interface
			m_pBuilderInterface=gtk_builder_new();
			gtk_builder_add_from_file(m_pBuilderInterface, OpenViBE::Directories::getDataDir() + "/../../../contrib/plugins/N400Experiment/N400.ui", NULL);
		
			if(!m_pBuilderInterface)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Couldn't load the interface !";
				return false;
			}

			gtk_builder_connect_signals(m_pBuilderInterface, NULL);

			m_pDrawingArea = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "N400DrawingArea"));
			gtk_widget_set_size_request(m_pDrawingArea, l_ui32WindowWidth, l_ui32WindowHeight);

			g_signal_connect(G_OBJECT(m_pDrawingArea), "key-press-event", G_CALLBACK(N400Experiment_KeyPressCallback), this);

			//set widget bg color
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oBackgroundColor);

			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oForegroundColor);

			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);

			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("Enter", 65293));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("+", 65451));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("-", 65453));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type(".", 46));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("00", 48));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("0", 65456));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("1", 65457));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("2", 65458));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("3", 65459));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("4", 65460));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("5", 65461));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("6", 65462));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("7", 65463));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("8", 65464));
			m_mButtonCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("9", 65465));

			// output stimulation
			m_oEncoder.initialize(*this, 0);
			m_oEncoder.encodeHeader();
			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);

			return true;
		}

		OpenViBE::boolean CN400Experiment::uninitialize()
		{

			//destroy drawing area
			if(m_pDrawingArea)
			{
				gtk_widget_destroy(m_pDrawingArea);
				m_pDrawingArea = NULL;
			}

			// unref the xml file as it's not needed anymore
			if(m_pBuilderInterface) 
			{
				g_object_unref(G_OBJECT(m_pBuilderInterface));
				m_pBuilderInterface=NULL;
			}

			// unref all pictures
			for (uint32 i = 0; i < m_pOriginalPicture.size(); i++) {
				if (m_pOriginalPicture[i].second)
				{
					g_object_unref(G_OBJECT(m_pOriginalPicture[i].second));
					m_pOriginalPicture[i].second = nullptr;
				}
				if (m_pScaledPicture[i].second)
				{
					g_object_unref(G_OBJECT(m_pScaledPicture[i].second));
					m_pScaledPicture[i].second = nullptr;
				}
			}

			m_oEncoder.uninitialize();

			return true;
		}

		OpenViBE::boolean CN400Experiment::processClock(CMessageClock& rMessageClock)
		{
			if (!m_bExperimentStarted) return true;

			// precomputing time variables
			static const uint64 l_ui64FirstPictureTime = m_ui64CrossDuration;
			static const uint64 l_ui64FirstPauseTime = l_ui64FirstPictureTime + m_ui64PictureDuration;
			static const uint64 l_ui64SecondPictureTime = l_ui64FirstPauseTime + m_ui64PauseDuration;
			static const uint64 l_ui64SecondPauseTime = l_ui64SecondPictureTime + m_ui64PictureDuration;
			static const uint64 l_ui64AnswerTime = l_ui64SecondPauseTime + 500;

			const uint64 l_ui64CurrentTime = rMessageClock.getTime();
			const uint64 l_ui64CurrenTimeMs = (uint64)((l_ui64CurrentTime >> 16) / 65.5360);

			if (m_bNewIteration) {
				m_eCurrentCue = CROSS;
				m_bRequestDraw = true;
				m_bNewIteration = false;
				m_ui64NewIterationTime = l_ui64CurrenTimeMs;
			}

			if (!m_bRequestDraw) {
				// First picture
				if ((m_eCurrentCue == PICTURE1) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64FirstPictureTime))
				{
					m_bRequestDraw = true;
				}
				// First pause
				else if ((m_eCurrentCue == PAUSE1) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64FirstPauseTime))
				{
					m_bRequestDraw = true;
				}
				// Second picture
				else if ((m_eCurrentCue == PICTURE2) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64SecondPictureTime))
				{
					m_bRequestDraw = true;
				}
				// Second pause
				else if ((m_eCurrentCue == PAUSE2) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64SecondPauseTime))
				{
					m_bRequestDraw = true;					
				}
				// Answer
				else if ((m_eCurrentCue == ANSWER) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64AnswerTime))
				{
					m_bProcessingKeys = true;
				}
			}

			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->clear();
			
			if (m_bRequestDraw && GTK_WIDGET(m_pDrawingArea)->window)
			{
				//gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,NULL,true);
				redraw();
				sendCurrentCue(m_ui64PreviousActivationTime, l_ui64CurrentTime);
			}

			if (m_bRequestProcessButton)
			{
				sendPressedButton(m_ui64PreviousActivationTime, l_ui64CurrentTime);
				m_bProcessingKeys = false;
				m_bRequestProcessButton = false;
				m_bNewIteration = true;
			}

			if (m_bRequestBeep)
			{
				sendBeep(m_ui64PreviousActivationTime, l_ui64CurrentTime);
				m_bRequestBeep = false;
			}

			m_ui64PreviousActivationTime = l_ui64CurrentTime;

			return true;
		}

		void CN400Experiment::sendStimulation(OpenViBE::uint64 ui64StimulationIdentifier, OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime)
		{
			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->appendStimulation(ui64StimulationIdentifier, ui64CurrentTime, 0);
			m_oEncoder.encodeBuffer();
			l_pBoxIO->markOutputAsReadyToSend(0, ui64PreviousTime, ui64CurrentTime);
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
		}

		void CN400Experiment::sendCurrentCue(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime)
		{
			sendStimulation(m_eCurrentCue, ui64PreviousTime, ui64CurrentTime);
		}

		void CN400Experiment::sendPressedButton(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime)
		{
			sendStimulation(m_ui32PressedButton, ui64PreviousTime, ui64CurrentTime);
		}

		void CN400Experiment::sendBeep(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime)
		{
			sendStimulation(OVTK_StimulationId_Beep, ui64PreviousTime, ui64CurrentTime);
		}

		// Callbacks

		void CN400Experiment::redraw()
		{
			if (!m_bRequestDraw) return;

			switch (m_eCurrentCue)
			{
				case CROSS:
					drawPicture(0);
					break;
				case PICTURE1:
				case PICTURE2:
					drawPicture(m_ui32RequestedPictureID++);
					if (m_ui32RequestedPictureID == m_pOriginalPicture.size()) m_ui32RequestedPictureID = 1;
					break;
				case PAUSE1:
				case PAUSE2:
					gdk_window_clear(m_pDrawingArea->window);
					break;
			}
			m_eCurrentCue = N400Cue((m_eCurrentCue + 1) % TOTAL_CUES);
			m_bRequestDraw = false;
		}

		/**
		 * Called when a key has been pressed.
		 * \param uiKey The gdk value to the pressed key.
		 * */
		void CN400Experiment::processKey(guint uiKey)
		{
			if (uiKey == 32) m_bExperimentStarted = true; // 32 = spacebar

			if (!m_bProcessingKeys) return;

			if (!validKey(uiKey))
			{
				m_bRequestBeep = true;
				return;
			}

			if (m_mButtonCodes[m_sRightButton] == uiKey) m_ui32PressedButton = OVTK_StimulationId_Label_0A;
			if (m_mButtonCodes[m_sWrongButton] == uiKey) m_ui32PressedButton = OVTK_StimulationId_Label_0B;
			if (m_mButtonCodes[m_sUnsureButton] == uiKey) m_ui32PressedButton = OVTK_StimulationId_Label_0C;

			m_bRequestProcessButton = true;
		}

		OpenViBE::boolean CN400Experiment::validKey(guint uiKey)
		{
			return m_mButtonCodes[m_sRightButton] == uiKey || 
				m_mButtonCodes[m_sWrongButton] == uiKey || 
				m_mButtonCodes[m_sUnsureButton] == uiKey;
		}

		void CN400Experiment::drawPicture(OpenViBE::uint32 uint32CueID)
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;
			gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pScaledPicture[uint32CueID].second, 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}
	};
};

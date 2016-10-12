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
		gboolean N400Experiment_SizeAllocateCallback(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
		{
			reinterpret_cast<CN400Experiment*>(data)->resize((uint32)allocation->width, (uint32)allocation->height);
			return FALSE;
		}

		gboolean N400Experiment_RedrawCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
		{
			std::cout << "registering redraw callback" << std::endl;
			reinterpret_cast<CN400Experiment*>(data)->redraw();
			return TRUE;
		}

		// Called when a key is pressed on the keyboard
		gboolean N400Experiment_KeyPressCallback(GtkWidget *widget, GdkEventKey *thisEvent, gpointer data)
		{
			std::cout << "registering key callback" << std::endl;
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
			m_ui32RightButtonCode(0),
			m_ui32WrongButtonCode(0),
			m_ui32UnsureButtonCode(0),
			m_bProcessingKeys(false),
			m_ui32PressedButton(0),
			m_bRequestProcessButton(false)
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

			// Image directory path
			CString l_sDirectoryPath;
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, l_sDirectoryPath);

			// Image extension
			CString l_sImageExtension;
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, l_sImageExtension);

			// Durations
			m_ui64CrossDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			m_ui64PictureDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			m_ui64PauseDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

			// Button codes
			// Numeric keyboard we are using sends 65457 for NUM1 so we need to remap it
			m_ui32RightButtonCode	= (OpenViBE::uint32)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5) + 65456;
			m_ui32WrongButtonCode	= (OpenViBE::uint32)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6) + 65456;
			m_ui32UnsureButtonCode	= (OpenViBE::uint32)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7) + 65456;

			// Load files inside directory
			path p(l_sDirectoryPath.toASCIIString());
			directory_iterator end_itr;

			for (directory_iterator itr(p); itr != end_itr; ++itr)
			{
				if (is_regular_file(itr->path()) && itr->path().extension() == ("." + l_sImageExtension).toASCIIString())
				{
					CString filename(itr->path().string().c_str());
					::GdkPixbuf* l_pOriginalPicture = gdk_pixbuf_new_from_file_at_size(filename, -1, -1, NULL);
					
					if (l_pOriginalPicture)
					{
						m_pOriginalPicture.push_back(std::make_pair(filename, l_pOriginalPicture));
						m_pScaledPicture.push_back(std::make_pair(filename, nullptr));
					}
					else
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Error couldn't load ressource file : " << filename << "!\n";
					}
				}
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
			g_signal_connect(G_OBJECT(m_pDrawingArea), "expose_event", G_CALLBACK(N400Experiment_RedrawCallback), this);
			g_signal_connect(G_OBJECT(m_pDrawingArea), "size-allocate", G_CALLBACK(N400Experiment_SizeAllocateCallback), this);
			g_signal_connect(G_OBJECT(m_pDrawingArea), "key-press-event", G_CALLBACK(N400Experiment_KeyPressCallback), this);

			//set widget bg color
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oBackgroundColor);

			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oForegroundColor);

			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);

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
			
			if (m_bRequestDraw && GTK_WIDGET(m_pDrawingArea)->window)
			{
				gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,NULL,true);
				sendCurrentCue(m_ui64PreviousActivationTime, l_ui64CurrentTime);
			}

			if (m_bRequestProcessButton)
			{
				sendPressedButton(m_ui64PreviousActivationTime, l_ui64CurrentTime);
				m_bProcessingKeys = false;
				m_bRequestProcessButton = false;
				m_bNewIteration = true;
			}

			m_ui64PreviousActivationTime = l_ui64CurrentTime;

			return true;
		}

		void CN400Experiment::sendStimulation(OpenViBE::uint64 ui64StimulationIdentifier, OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime)
		{
			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->clear();		// The encoder may retain the buffer from the previous round, clear it
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

		// Callbacks

		void CN400Experiment::redraw()
		{
			if (!m_bRequestDraw) return;

			switch (m_eCurrentCue)
			{
				case CROSS:
					drawCuePicture(0);
					break;
				case PICTURE1:
				case PICTURE2:
					drawCuePicture(m_ui32RequestedPictureID++);
					if (m_ui32RequestedPictureID == m_pOriginalPicture.size()) m_ui32RequestedPictureID = 1;
					break;
			}
			m_eCurrentCue = N400Cue((m_eCurrentCue + 1) % TOTAL_CUES);
			m_bRequestDraw = false;
		}

		void CN400Experiment::resize(uint32 ui32Width, uint32 ui32Height)
		{
			for (uint32 i = 0; i < m_pScaledPicture.size(); i++) {
				if (m_pScaledPicture[i].second) {
					g_object_unref(G_OBJECT(m_pScaledPicture[i].second));
				}
				m_pScaledPicture[i].second = gdk_pixbuf_scale_simple(m_pOriginalPicture[i].second, ui32Width, ui32Height, GDK_INTERP_BILINEAR);
			}
		}

		/**
		 * Called when a key has been pressed.
		 * \param uiKey The gdk value to the pressed key.
		 * */
		void CN400Experiment::processKey(guint uiKey)
		{
			if (!m_bProcessingKeys) return;

			m_ui32PressedButton = uiKey;
			m_bRequestProcessButton = true;
		}

		void CN400Experiment::drawCuePicture(OpenViBE::uint32 uint32CueID)
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;
			gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pScaledPicture[uint32CueID].second, 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}
	};
};

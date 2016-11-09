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
			// std::cout << thisEvent->keyval << std::endl;
			reinterpret_cast<CN400Experiment*>(data)->processKey(thisEvent->keyval);
			return true;
		}

		std::string obtainFilename(std::string path)
		{
			return path.substr(path.rfind("_") + 1);
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
			m_ui64FirstPauseDuration(0),
			m_ui64SecondPauseDuration(0),
			m_ui64ThirdPauseDuration(0),
			m_pMainWindow(NULL),
			m_ui32RequestedPictureID(1),
			m_bNewIteration(true),
			m_eCurrentCue(CROSS),
			m_ui64PreviousTime(0),
			m_ui64CurrentTime(0),
			m_ui64NewIterationTime(0),
			m_sMatchingButton(NULL),
			m_sNonmatchingButton(NULL),
			m_sUnsureButton(NULL),
			m_bProcessingKeys(false),
			m_bExperimentStarted(false)
		{}

		OpenViBE::boolean CN400Experiment::initialize()
		{
			//>>>> Reading Settings:

			// Window size
			m_ui32PictureWidth			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			m_ui32PictureHeight			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			// Durations
			m_ui64CrossDuration			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			m_ui64FirstPauseDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			m_ui64PictureDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
			m_ui64SecondPauseDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
			m_ui64ThirdPauseDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);

			// Button codes
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(7, m_sMatchingButton);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(8, m_sNonmatchingButton);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(9, m_sUnsureButton);

			// Experiment directory and iteration
			OpenViBE::CString l_sExperimentDirectory;
			OpenViBE::CString l_sExperimentIteration;
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(10, l_sExperimentDirectory);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(11, l_sExperimentIteration);

			// Get experiment directory
			
			if (l_sExperimentDirectory != OpenViBE::CString("training"))
			{
				l_sExperimentDirectory = l_sExperimentDirectory + "/" + l_sExperimentIteration;
			}
			l_sExperimentDirectory = OpenViBE::Directories::getDataDir() + "/../../../n400/" + l_sExperimentDirectory;
			if (!loadDataset(l_sExperimentDirectory))
			{
				return false;
			}
			// we need to know whether picture and following word match
			createMatches();

			m_pMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			if (!m_pMainWindow)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Couldn't create window!";
				return false;
			}
			gtk_window_set_title(GTK_WINDOW(m_pMainWindow), "N400 Experiment");
			gtk_widget_set_usize(m_pMainWindow, 600, 600);
			gtk_window_set_gravity(GTK_WINDOW(m_pMainWindow), GDK_GRAVITY_CENTER);

			// set background color of main window
			GdkColor u_oBackgroundColor;
			u_oBackgroundColor.pixel = 0;
			u_oBackgroundColor.red = 0xFFFF;
			u_oBackgroundColor.green = 0xFFFF;
			u_oBackgroundColor.blue = 0xFFFF;
			gtk_widget_modify_bg(m_pMainWindow, GTK_STATE_NORMAL, &u_oBackgroundColor);

			gtk_widget_show(m_pMainWindow);
			g_signal_connect(m_pMainWindow, "key-press-event", G_CALLBACK(N400Experiment_KeyPressCallback), this);

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

			return true;
		}

		OpenViBE::boolean CN400Experiment::uninitialize()
		{

			//destroy main window
			if(m_pMainWindow)
			{
				gtk_widget_destroy(m_pMainWindow);
				m_pMainWindow = NULL;
			}

			// unref all pictures
			for (uint32 i = 0; i < m_vImagesDataset.size(); i++) {
				if (m_vImagesDataset[i].second)
				{
					g_object_unref(G_OBJECT(m_vImagesDataset[i].second));
					m_vImagesDataset[i].second = nullptr;
				}
			}

			m_oEncoder.uninitialize();

			return true;
		}

		OpenViBE::boolean CN400Experiment::processClock(CMessageClock& rMessageClock)
		{
			if (!m_bExperimentStarted) return true;

			// precomputing time variables
			const uint64 l_ui64FirstPauseTime = m_ui64CrossDuration;
			const uint64 l_ui64FirstPictureTime = l_ui64FirstPauseTime + m_ui64FirstPauseDuration;
			const uint64 l_ui64SecondPauseTime = l_ui64FirstPictureTime + m_ui64PictureDuration;
			const uint64 l_ui64SecondPictureTime = l_ui64SecondPauseTime + m_ui64SecondPauseDuration;
			const uint64 l_ui64ThirdPauseTime = l_ui64SecondPictureTime + m_ui64PictureDuration;
			const uint64 l_ui64AnswerTime = l_ui64ThirdPauseTime + m_ui64ThirdPauseDuration;

			m_ui64CurrentTime = rMessageClock.getTime();
			const uint64 l_ui64CurrenTimeMs = (uint64)((m_ui64CurrentTime >> 16) / 65.5360);

			if (m_bNewIteration) {
				if (m_ui32RequestedPictureID == m_vImagesDataset.size()) // were all pictures used?
				{
					// stop experiment
					m_vStimulationsToSend.push_back(OVTK_StimulationId_ExperimentStop);
					sendStimulations();
					return true;
				}
				m_eCurrentCue = CROSS;
				m_bNewIteration = false;
				m_ui64NewIterationTime = l_ui64CurrenTimeMs;
			}

			// Cross
			if (m_eCurrentCue == CROSS)
			{
				redraw();
			}
			else if ((m_eCurrentCue == PAUSE1) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64FirstPauseTime))
			{
				redraw();
			}
			// First picture
			else if ((m_eCurrentCue == PICTURE1) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64FirstPictureTime))
			{
				redraw();
			}
			// Second pause
			else if ((m_eCurrentCue == PAUSE2) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64SecondPauseTime))
			{
				redraw();
			}
			// Second picture
			else if ((m_eCurrentCue == PICTURE2) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64SecondPictureTime))
			{
				redraw();
			}
			// Third pause
			else if ((m_eCurrentCue == PAUSE3) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64ThirdPauseTime))
			{
				redraw();
			}
			// Answer
			else if ((m_eCurrentCue == ANSWER) && (l_ui64CurrenTimeMs >= m_ui64NewIterationTime + l_ui64AnswerTime))
			{
				m_bProcessingKeys = true;
			}

			sendStimulations();
			m_ui64PreviousTime = m_ui64CurrentTime;

			return true;
		}

		OpenViBE::boolean CN400Experiment::loadDataset(OpenViBE::CString experimentDirectory)
		{
			path p(experimentDirectory.toASCIIString());
			directory_iterator end_itr;

			for (directory_iterator itr(p); itr != end_itr; ++itr)
			{
				if (is_regular_file(itr->path()))
				{
					CString filename(itr->path().string().c_str());
					::GdkPixbuf* l_pOriginalPicture = gdk_pixbuf_new_from_file_at_size(filename, -1, -1, NULL);
					::GdkPixbuf* l_pScaledPicture = gdk_pixbuf_scale_simple(l_pOriginalPicture, m_ui32PictureWidth, m_ui32PictureHeight, GDK_INTERP_BILINEAR);
					if (!l_pScaledPicture)
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error couldn't load ressource file : " << filename << "!\n";
						return false;
					}
                    g_object_unref(G_OBJECT(l_pOriginalPicture));
                    l_pOriginalPicture = nullptr;
					m_vImagesDataset.push_back(std::make_pair(filename, l_pScaledPicture));
				}
			}

			if (m_vImagesDataset.empty()){
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error, no images in directory!";
				return false;
			}

			// Sort files according to digits in beginning of filename
			std::sort(m_vImagesDataset.begin(), m_vImagesDataset.end(), filenamesCompare);
			return true;
		}

		void CN400Experiment::createMatches()
		{
			m_vMatches.push_back(true); // first one is cross which we dont care about
			for (uint32 i = 1; i < m_vImagesDataset.size(); i+= 2)
			{
				std::string filenamePicture = obtainFilename(m_vImagesDataset[i].first.toASCIIString());
				std::string filenameWord = obtainFilename(m_vImagesDataset[i+1].first.toASCIIString());

				OpenViBE::boolean match = filenamePicture[0] == filenameWord[0];
				m_vMatches.push_back(match); // for picture
				m_vMatches.push_back(match); // for word
			}
		}

		void CN400Experiment::sendStimulations()
		{
			if (m_vStimulationsToSend.size() == 0) return;

			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->clear();
			for (uint32 i = 0; i < m_vStimulationsToSend.size(); i++)
			{
				l_pStimulationSet->appendStimulation(m_vStimulationsToSend[i], m_ui64CurrentTime, 0);
			}
			m_vStimulationsToSend.clear();
			m_oEncoder.encodeBuffer();
			l_pBoxIO->markOutputAsReadyToSend(0, m_ui64PreviousTime, m_ui64CurrentTime);
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
		}

		// Callbacks

		void CN400Experiment::redraw()
		{
			uint32 l_ui32Match = 0;
			if (m_ui32RequestedPictureID < m_vImagesDataset.size())
			{
				l_ui32Match = m_vMatches[m_ui32RequestedPictureID] ? N400_PIC_MATCH - 1 : N400_PIC_MISMATCH - 1;
			}

			switch (m_eCurrentCue)
			{
				case CROSS:
					m_vStimulationsToSend.push_back(N400_CROSS);
					drawPicture(0);
					break;
				case PICTURE1:
					m_vStimulationsToSend.push_back(l_ui32Match + 1);
					drawPicture(m_ui32RequestedPictureID);
					m_ui32RequestedPictureID++;
					break;
				case PICTURE2:
					m_vStimulationsToSend.push_back(l_ui32Match + 3);
					drawPicture(m_ui32RequestedPictureID);
					m_ui32RequestedPictureID++;
					break;
				case PAUSE1:
					m_vStimulationsToSend.push_back(N400_1ST_PAUSE);
					gdk_window_clear(m_pMainWindow->window);
					break;
				case PAUSE2:
					m_vStimulationsToSend.push_back(l_ui32Match + 2);
					gdk_window_clear(m_pMainWindow->window);
					break;
				case PAUSE3:
					m_vStimulationsToSend.push_back(N400_3RD_PAUSE);
					gdk_window_clear(m_pMainWindow->window);
					break;
			}
			m_eCurrentCue = N400Cue((m_eCurrentCue + 1) % TOTAL_CUES);
		}

		/**
		 * Called when a key has been pressed.
		 * \param uiKey The gdk value to the pressed key.
		 * */
		void CN400Experiment::processKey(guint uiKey)
		{
			if (!m_bExperimentStarted && (uiKey == 32)) // 32 = keycode of Spacebar
			{
				m_bExperimentStarted = true;
				gtk_window_set_decorated(GTK_WINDOW(m_pMainWindow), false);
				gtk_window_maximize(GTK_WINDOW(m_pMainWindow));
			}

			if (!m_bProcessingKeys) return;

			if (!validKey(uiKey))
			{
				m_vStimulationsToSend.push_back(OVTK_StimulationId_Beep);
				return;
			}

			if (m_mButtonCodes[m_sMatchingButton] == uiKey)
			{
				m_vStimulationsToSend.push_back(N400_ANSWER_MATCH);
				m_vStimulationsToSend.push_back(N400_CORRECT_ANSWER + !m_vMatches[m_ui32RequestedPictureID - 1]);
			}
			if (m_mButtonCodes[m_sNonmatchingButton] == uiKey)
			{
				m_vStimulationsToSend.push_back(N400_ANSWER_NO_MATCH);
				m_vStimulationsToSend.push_back(N400_CORRECT_ANSWER + m_vMatches[m_ui32RequestedPictureID - 1]);
			}
			if (m_mButtonCodes[m_sUnsureButton] == uiKey)
			{
				m_vStimulationsToSend.push_back(N400_ANSWER_UNSURE);
			}

			m_bProcessingKeys = false;
			m_bNewIteration = true;
		}

		OpenViBE::boolean CN400Experiment::validKey(guint uiKey)
		{
			return m_mButtonCodes[m_sMatchingButton] == uiKey || 
				m_mButtonCodes[m_sNonmatchingButton] == uiKey || 
				m_mButtonCodes[m_sUnsureButton] == uiKey;
		}

		void CN400Experiment::drawPicture(OpenViBE::uint32 uint32CueID)
		{
			gint l_iImagePosX = (m_pMainWindow->allocation.width - m_ui32PictureWidth) / 2;
			gint l_iImagePosY = (m_pMainWindow->allocation.height - m_ui32PictureHeight) / 2;
			GdkPixbuf* image = m_vImagesDataset[uint32CueID].second;
			gdk_draw_pixbuf(m_pMainWindow->window, NULL, image, 0, 0, l_iImagePosX, l_iImagePosY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}
	};
};

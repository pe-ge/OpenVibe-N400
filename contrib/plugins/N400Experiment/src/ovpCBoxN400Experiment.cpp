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
			m_sRightButton(NULL),
			m_sWrongButton(NULL),
			m_sUnsureButton(NULL),
			m_bProcessingKeys(false),
			m_ui32PressedButton(0),
			m_bRequestProcessButton(false),
			m_bRequestBeep(false),
			m_bExperimentStarted(false),
			m_bDataset1Chosen(true)
		{}

		OpenViBE::boolean CN400Experiment::initialize()
		{
			//>>>> Reading Settings:

			// Window size
			m_ui32PictureWidth			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			m_ui32PictureHeight			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			// Durations
			m_ui64CrossDuration			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			m_ui64PictureDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			m_ui64FirstPauseDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
			m_ui64SecondPauseDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
			m_ui64ThirdPauseDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);

			// Button codes
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(7, m_sRightButton);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(8, m_sWrongButton);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(9, m_sUnsureButton);

			// Get experiment directory
			std::string l_sExperimentDirectory = OpenViBE::Directories::getDataDir() + "/../../../n400/experiment";
			OpenViBE::uint64 l_ui64NumExperiment = 1;
			while (boost::filesystem::exists(l_sExperimentDirectory + std::to_string(l_ui64NumExperiment)))
			{
				l_ui64NumExperiment++;
			}
			l_ui64NumExperiment--;
			l_sExperimentDirectory += std::to_string(l_ui64NumExperiment);

			OpenViBE::boolean result = true;
			result &= loadDataset(l_sExperimentDirectory + "/1", m_vDataset1);
			result &= loadDataset(l_sExperimentDirectory + "/2", m_vDataset2);
			if (!result)
			{
				return false;
			}

			if (m_vDataset1.size() != m_vDataset2.size())
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error, dataset 1 and 2 differ in number of images!";
				return false;
			}

			///////////////////////////////////////////////////////
			m_pMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			if (!m_pMainWindow)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Couldn't create window!";
				return false;
			}
			gtk_window_set_title(GTK_WINDOW(m_pMainWindow), "N400 Experiment");
			gtk_widget_set_usize(m_pMainWindow, 600, 600);
			gtk_window_set_gravity(GTK_WINDOW(m_pMainWindow), GDK_GRAVITY_CENTER);

			// set background color
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

			// we need to know whether picture and following word match
			createDatasetOfMatches(m_vDatasetOfMatches1, m_vDataset1);
			createDatasetOfMatches(m_vDatasetOfMatches2, m_vDataset2);

			// output stimulation
			m_oEncoder.initialize(*this, 0);
			m_oEncoder.encodeHeader();
			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);

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
			deleteDataset(m_vDataset1);
			deleteDataset(m_vDataset2);

			m_oEncoder.uninitialize();

			return true;
		}

		OpenViBE::boolean CN400Experiment::processClock(CMessageClock& rMessageClock)
		{
			if (!m_bExperimentStarted) return true;

			// clear previous stimulations
			// do we need this?
			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->clear();

			// precomputing time variables
			const uint64 l_ui64FirstPauseTime = m_ui64CrossDuration;
			const uint64 l_ui64FirstPictureTime = l_ui64FirstPauseTime + m_ui64FirstPauseDuration;
			const uint64 l_ui64SecondPauseTime = l_ui64FirstPictureTime + m_ui64PictureDuration;
			const uint64 l_ui64SecondPictureTime = l_ui64SecondPauseTime + m_ui64SecondPauseDuration;
			const uint64 l_ui64ThirdPauseTime = l_ui64SecondPictureTime + m_ui64PictureDuration;
			const uint64 l_ui64AnswerTime = l_ui64ThirdPauseTime + m_ui64ThirdPauseDuration;

			const uint64 m_ui64CurrentTime = rMessageClock.getTime();
			const uint64 l_ui64CurrenTimeMs = (uint64)((m_ui64CurrentTime >> 16) / 65.5360);

			if (m_bNewIteration) {
				if (m_ui32RequestedPictureID == m_vDataset1.size()) // were all pictures used?
				{
					// stop experiment
					sendStimulation(OVTK_StimulationId_ExperimentStop);
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

			if (m_bRequestProcessButton)
			{
				sendStimulation(m_ui32PressedButton);
				m_bProcessingKeys = false;
				m_bRequestProcessButton = false;
				m_bNewIteration = true;
			}

			if (m_bRequestBeep)
			{
				sendStimulation(OVTK_StimulationId_Beep);
				m_bRequestBeep = false;
			}

			m_ui64PreviousTime = m_ui64CurrentTime;

			return true;
		}

		OpenViBE::boolean CN400Experiment::loadDataset(std::string experimentDirectory, std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>>& dataset)
		{
			path p(experimentDirectory);
			directory_iterator end_itr;

			for (directory_iterator itr(p); itr != end_itr; ++itr)
			{
				if (is_regular_file(itr->path()))
				{
					CString filename(itr->path().string().c_str());
					::GdkPixbuf* l_pOriginalPicture = gdk_pixbuf_new_from_file_at_size(filename, -1, -1, NULL);
					::GdkPixbuf* l_pScaledPicture = gdk_pixbuf_scale_simple(l_pOriginalPicture, m_ui32PictureWidth, m_ui32PictureHeight, GDK_INTERP_BILINEAR);
					if (l_pOriginalPicture)
					{
						dataset.push_back(std::make_pair(filename, l_pScaledPicture));
					}
					else
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error couldn't load ressource file : " << filename << "!\n";
						return false;
					}
				}
			}

			if (dataset.empty()){
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error, no images in directory!";
				return false;
			}

			// Sort files according to digits in beginning of filename
			std::sort(dataset.begin(), dataset.end(), filenamesCompare);
			return true;
		}

		void CN400Experiment::deleteDataset(std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>>& dataset)
		{
			for (uint32 i = 0; i < dataset.size(); i++) {
				if (dataset[i].second)
				{
					g_object_unref(G_OBJECT(dataset[i].second));
					dataset[i].second = nullptr;
				}
			}
		}

		void CN400Experiment::createDatasetOfMatches(std::vector<OpenViBE::boolean>& datasetOfMatches, std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>>& imageDataset)
		{
			datasetOfMatches.push_back(true); // first one is cross which we dont care about
			for (uint32 i = 1; i < imageDataset.size(); i+= 2)
			{
				std::string filenamePicture = obtainFilename(imageDataset[i].first.toASCIIString());
				std::string filenameWord = obtainFilename(imageDataset[i+1].first.toASCIIString());

				OpenViBE::boolean match = filenamePicture[0] == filenameWord[0];
				datasetOfMatches.push_back(match); // for picture
				datasetOfMatches.push_back(match); // for word
			}
		}

		void CN400Experiment::sendStimulation(OpenViBE::uint64 ui64StimulationIdentifier)
		{
			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->appendStimulation(ui64StimulationIdentifier, m_ui64CurrentTime, 0);
			m_oEncoder.encodeBuffer();
			l_pBoxIO->markOutputAsReadyToSend(0, m_ui64PreviousTime, m_ui64CurrentTime);
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
		}

		// Callbacks

		void CN400Experiment::redraw()
		{
			std::vector<OpenViBE::boolean>& l_vChosenDatasetOfMatches = m_bDataset1Chosen ? m_vDatasetOfMatches1 : m_vDatasetOfMatches2;
			uint32 l_ui32Match = 100;
			if (m_ui32RequestedPictureID < l_vChosenDatasetOfMatches.size())
			{
				l_ui32Match = l_vChosenDatasetOfMatches[m_ui32RequestedPictureID] ? 200 : 300;
			}

			switch (m_eCurrentCue)
			{
				case CROSS:
					sendStimulation(0);
					drawPicture(0);
					break;
				case PICTURE1:
					sendStimulation(l_ui32Match + 1);
					drawPicture(m_ui32RequestedPictureID);
					m_ui32RequestedPictureID++;
					break;
				case PICTURE2:
					sendStimulation(l_ui32Match + 3);
					drawPicture(m_ui32RequestedPictureID);
					m_ui32RequestedPictureID++;
					break;
				case PAUSE1:
					sendStimulation(1);
					gdk_window_clear(m_pMainWindow->window);
					break;
				case PAUSE2:
					sendStimulation(l_ui32Match + 2);
					gdk_window_clear(m_pMainWindow->window);
					break;
				case PAUSE3:
					sendStimulation(4);
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
			if (!m_bExperimentStarted && (uiKey == 49 || uiKey == 50)) // 49 = keycode of NUM1, 50 = NUM2
			{
				m_bExperimentStarted = true;
				m_bDataset1Chosen = (uiKey == 49);
				gtk_window_set_decorated(GTK_WINDOW(m_pMainWindow), false);
				gtk_window_maximize(GTK_WINDOW(m_pMainWindow));
			}

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
			gint l_iImagePosX = (m_pMainWindow->allocation.width - m_ui32PictureWidth) / 2;
			gint l_iImagePosY = (m_pMainWindow->allocation.height - m_ui32PictureHeight) / 2;
			GdkPixbuf* image = m_bDataset1Chosen ? m_vDataset1[uint32CueID].second : m_vDataset2[uint32CueID].second;
			gdk_draw_pixbuf(m_pMainWindow->window, NULL, image, 0, 0, l_iImagePosX, l_iImagePosY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}
	};
};

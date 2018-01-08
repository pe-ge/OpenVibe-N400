#include "ovpCBoxN400Sentences24.h"

#include "boost/locale.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem.hpp"
#include <fstream>
#include <locale>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::N400S;

using namespace boost::filesystem;

namespace OpenViBEPlugins
{
	namespace N400S
	{
		// Called when a key is pressed on the keyboard
		gboolean N400Sentences_KeyPressCallback(GtkWidget *widget, GdkEventKey *thisEvent, gpointer data)
		{
			reinterpret_cast<CN400Sentences24*>(data)->processKey(thisEvent->keyval);
			return true;
		}

		CN400Sentences24::CN400Sentences24(void) :
			m_ui64StartingCueDuration(0),
			m_ui64WordDuration(0),
			m_ui32FontSize(0),
			m_ui32TotalObservedSentences(0),
			m_ui32NumNewSentences(0),
			m_ui32NumOldSentences(0),
			m_sKeyCorrect(NULL),
			m_sKeyIncorrect(NULL),
			m_sStartCue(NULL),
			m_sAnswerCue(NULL),
			m_pMainWindow(NULL),
			m_pStartCueLabel(NULL),
			m_pAnswerCueLabel(NULL),
			m_pPrevLabel(NULL),
			m_ui32SentenceId(0),
			m_ui32WordId(0),
			m_bExperimentStarted(false),
			m_bProcessingKeys(false),
			m_ui64PreviousTime(0),
			m_ui64CurrentTime(0),
			m_ui64StartSentenceTime(0),
			m_bNewIteration(true),
			m_eCurrentCue(INIT_CUE)
		{}

		OpenViBE::boolean CN400Sentences24::initialize()
		{
			// Experiment directory and iteration
			OpenViBE::CString l_sN400Sdir;
			OpenViBE::CString l_sDataset;

			// Obtain box settings
			m_ui64StartingCueDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			m_ui64WordDuration				= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(2, m_sKeyCorrect);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(3, m_sKeyIncorrect);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(4, l_sN400Sdir);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(5, l_sDataset);
			m_ui32FontSize					= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
			m_ui32TotalObservedSentences	= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
			m_ui32NumNewSentences			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
			m_ui32NumOldSentences			= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(10, m_sStartCue);
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(11, m_sAnswerCue);

			if (m_ui32TotalObservedSentences < m_ui32NumOldSentences) {
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Total observed sentences must be greater than number of old sentences";
				return false;
			}

			// create main window
			m_pMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			if (!m_pMainWindow)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Couldn't create window!";
				return false;
			}
			gtk_window_set_title(GTK_WINDOW(m_pMainWindow), "N400 Sentences");
			gtk_widget_set_usize(m_pMainWindow, 600, 600);
			gtk_window_set_gravity(GTK_WINDOW(m_pMainWindow), GDK_GRAVITY_CENTER);

			// set background color of main window
			GdkColor u_oBackgroundColor;
			u_oBackgroundColor.pixel = 0;
			u_oBackgroundColor.red = 0xC8C8;
			u_oBackgroundColor.green = 0xC8C8;
			u_oBackgroundColor.blue = 0xC8C8;
			gtk_widget_modify_bg(m_pMainWindow, GTK_STATE_NORMAL, &u_oBackgroundColor);

			g_signal_connect(m_pMainWindow, "key-press-event", G_CALLBACK(N400Sentences_KeyPressCallback), this);

			m_mKeyCodes["Enter"] = 65293;
			m_mKeyCodes["+"] = 65451;
			m_mKeyCodes["-"] = 65453;
			m_mKeyCodes["."] = 46;
			m_mKeyCodes["00"] = 48;
			m_mKeyCodes["0"] = 65456;
			m_mKeyCodes["1"] = 65457;
			m_mKeyCodes["2"] = 65458;
			m_mKeyCodes["3"] = 65459;
			m_mKeyCodes["4"] = 65460;
			m_mKeyCodes["5"] = 65461;
			m_mKeyCodes["6"] = 65462;
			m_mKeyCodes["7"] = 65463;
			m_mKeyCodes["8"] = 65464;
			m_mKeyCodes["9"] = 65465;

			/*m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("Enter", 65293));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("+", 65451));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("-", 65453));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type(".", 46));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("00", 48));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("0", 65456));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("1", 65457));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("2", 65458));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("3", 65459));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("4", 65460));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("5", 65461));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("6", 65462));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("7", 65463));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("8", 65464));
			m_mKeyCodes.insert(std::map<OpenViBE::CString, OpenViBE::uint32>::value_type("9", 65465));*/

			// init output stimulations
			m_oEncoder.initialize(*this, 0);
			m_oEncoder.encodeHeader();

			// load dataset
			if (!prepareDataset(l_sN400Sdir + "/" + l_sDataset)) {
				return false;
			}

			m_pStartCueLabel = createLabel(m_sStartCue.toASCIIString());
			m_pAnswerCueLabel = createLabel(m_sAnswerCue.toASCIIString());

			gtk_widget_show(m_pMainWindow);
			return true;
		}

		OpenViBE::boolean CN400Sentences24::uninitialize()
		{
			// unref all labels
			g_object_unref(m_pStartCueLabel);
			g_object_unref(m_pAnswerCueLabel);
			for (unsigned int i = 0; i < m_vSentences.size(); i++) {
				for (unsigned int j = 0; j < m_vSentences[i].first.size(); j++) {
					g_object_unref(m_vSentences[i].first[j]);
				}
			}
			// destroy main window
			if(m_pMainWindow)
			{
				gtk_widget_destroy(m_pMainWindow);
				m_pMainWindow = NULL;
			}

			m_oEncoder.uninitialize();

			return true;
		}

		OpenViBE::boolean CN400Sentences24::processClock(CMessageClock& rMessageClock)
		{
			if (!m_bExperimentStarted) return true;

			m_ui64CurrentTime = rMessageClock.getTime();
			const uint64 l_ui64CurrenTimeMs = (uint64)((m_ui64CurrentTime >> 16) / 65.5360);

			// if start of the new sentence
			if (m_bNewIteration) {
				// if all sentences used => stop
				if (m_ui32SentenceId == m_vSentences.size()) {
					m_vStimulationsToSend.push_back(OVTK_StimulationId_ExperimentStop);
					sendStimulations();
					return true;
				}

				m_eCurrentCue = INIT_CUE;
				m_bNewIteration = false;
				m_ui64StartSentenceTime = l_ui64CurrenTimeMs;
			}

			// show cross
			if (m_eCurrentCue == INIT_CUE) {
				showLabel(m_pStartCueLabel);

				// send sentence type
				m_vStimulationsToSend.push_back(m_vSentences[m_ui32SentenceId].second);
				m_eCurrentCue = WORD;
			}

			// show words
			if (m_eCurrentCue == WORD && l_ui64CurrenTimeMs >= m_ui64StartSentenceTime + m_ui64StartingCueDuration + m_ui32WordId * m_ui64WordDuration) {
				showLabel(m_vSentences[m_ui32SentenceId].first[m_ui32WordId]);
				m_vStimulationsToSend.push_back(N400S_WORD);
				m_ui32WordId++;

				// if end of sentence
				if (m_ui32WordId == m_vSentences[m_ui32SentenceId].first.size()) {
					// if all observed sentences were used
					if (m_ui32SentenceId + 1 >= m_ui32TotalObservedSentences) {
						m_eCurrentCue = ANSWER;
					} else {
						m_bNewIteration = true;
					}
				}
			}

			// last word was shown => requesting participants answer
			if (m_eCurrentCue == ANSWER && !m_bProcessingKeys && l_ui64CurrenTimeMs >= m_ui64StartSentenceTime + m_ui64StartingCueDuration + m_ui32WordId * m_ui64WordDuration) {
				showLabel(m_pAnswerCueLabel);
				m_vStimulationsToSend.push_back(N400S_ANSWER_REQUEST);
				m_bProcessingKeys = true;
			}

			sendStimulations();
			m_ui64PreviousTime = m_ui64CurrentTime;

			return true;
		}

		OpenViBE::boolean CN400Sentences24::prepareDataset(OpenViBE::CString datasetPath)
		{
			// allow UTF-8 filenames
			std::locale::global(boost::locale::generator().generate(""));
			boost::filesystem::path::imbue(std::locale());

			// open file
			std::ifstream infile(datasetPath);

			std::vector<std::string> hand_related_ok;
			std::vector<std::string> hand_related_bad;
			std::vector<std::string> non_action_ok;
			std::vector<std::string> non_action_bad;

			if (infile) {
				std::string line;
				// skip first line (header)
				std::getline(infile, line);
				while (std::getline(infile, line)) {
					std::istringstream instream(line);
					if (instream) {
						std::string sentence;

						// hand related ok
						std::getline(instream, sentence, '\t');
						hand_related_ok.push_back(sentence);

						// skip verb meaning
						std::getline(instream, sentence, '\t');

						// hand related bad
						std::getline(instream, sentence, '\t');
						hand_related_bad.push_back(sentence);

						// non action ok
						std::getline(instream, sentence, '\t');
						non_action_ok.push_back(sentence);

						// non action bad
						std::getline(instream, sentence, '\t');
						non_action_bad.push_back(sentence);
					} else {
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Cannot parse line: " << line.c_str() << "!\n";
						return false;
					}
				}
			} else {
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Cannot open file: " << datasetPath << "!\n";
				return false;
			}

			// shuffle everything

			std::srand((unsigned int)time(0)); // set seed
			std::random_shuffle(hand_related_ok.begin(), hand_related_ok.end());
			std::random_shuffle(hand_related_bad.begin(), hand_related_bad.end());
			std::random_shuffle(non_action_ok.begin(), non_action_ok.end());
			std::random_shuffle(non_action_bad.begin(), non_action_bad.end());

			// put all sentences into one vector
			std::vector<std::vector<::GtkWidget*>> all_sentences;
			for (unsigned int i = 0; i < hand_related_ok.size(); i++) {
				std::vector<std::string> words;

				all_sentences.push_back(createWords(hand_related_ok[i]));
				all_sentences.push_back(createWords(hand_related_bad[i]));
				all_sentences.push_back(createWords(non_action_ok[i]));
				all_sentences.push_back(createWords(non_action_bad[i]));
			}

			std::random_shuffle(all_sentences.begin(), all_sentences.end());

			std::vector<std::pair<std::vector<::GtkWidget*>, OpenViBE::boolean>> l_vObservedSentences;
			std::vector<std::pair<std::vector<::GtkWidget*>, OpenViBE::boolean>> l_vAskedSentences;
			// take observed sentences
			for (unsigned int i = 0; i < m_ui32TotalObservedSentences; i++) {
				l_vObservedSentences.push_back(std::make_pair(all_sentences[i], false));
			}

			// take old sentences
			for (unsigned int i = 0; i < m_ui32NumOldSentences; i++) {
				l_vAskedSentences.push_back(std::make_pair(all_sentences[i], false));
			}

			// take new sentences
			for (unsigned int i = m_ui32TotalObservedSentences; i < m_ui32TotalObservedSentences + m_ui32NumNewSentences; i++) {
				l_vAskedSentences.push_back(std::make_pair(all_sentences[i], true));
			}

			std::random_shuffle(l_vObservedSentences.begin(), l_vObservedSentences.end());
			std::random_shuffle(l_vAskedSentences.begin(), l_vAskedSentences.end());

			// create final vector of sentences
			for (unsigned int i = 0; i < l_vObservedSentences.size(); i++) {
				m_vSentences.push_back(l_vObservedSentences[i]);
			}

			for (unsigned int i = 0; i < l_vAskedSentences.size(); i++) {
				m_vSentences.push_back(l_vAskedSentences[i]);
			}

			return true;
		}

		std::vector<::GtkWidget*> CN400Sentences24::createWords(std::string sentence) {
			std::vector<::GtkWidget*> words;

			std::string word;
			std::istringstream instream(sentence);
			while (std::getline(instream, word, ' ')) {
				words.push_back(createLabel(word));
			}

			return words;
		}

		::GtkWidget* CN400Sentences24::createLabel(std::string text) {
			::GtkWidget* label = gtk_label_new("");
			// ref label so its not destroyed when removed from container
			g_object_ref(label);
			// copy text into char array
			char buff[100];
			sprintf_s(buff, sizeof(buff), "<span font_desc=\"%d\">%s</span>", m_ui32FontSize, text.c_str());
			// set text of the label with given font size
			gtk_label_set_markup((GtkLabel *) label, buff); 
			return label;
		}

		void CN400Sentences24::sendStimulations()
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

		void CN400Sentences24::showLabel(::GtkWidget* label)
		{
			// remove previous label
			if (m_pPrevLabel) gtk_container_remove(GTK_CONTAINER(m_pMainWindow), m_pPrevLabel);

			// add new label
			gtk_container_add(GTK_CONTAINER(m_pMainWindow), label);
			gtk_widget_show(label);

			// set previous label
			m_pPrevLabel = label;
		}
		void CN400Sentences24::processKey(guint uiKey)
		{
			// first key press starts experiment
			if (!m_bExperimentStarted)
			{
				m_bExperimentStarted = true;
				gtk_window_set_decorated(GTK_WINDOW(m_pMainWindow), false);
				//gtk_window_maximize(GTK_WINDOW(m_pMainWindow));
				return;
			}

			// if not processing keys, exit
			if (!m_bProcessingKeys) {
				m_vStimulationsToSend.push_back(N400S_ANSWER_INVALID);
				return;
			}

			//
			if (!validKey(uiKey)) {
				m_vStimulationsToSend.push_back(N400S_ANSWER_INVALID);
				return;
			}

			// determine whether answer is correct
			OpenViBE::boolean pressedOk = (m_mKeyCodes[m_sKeyCorrect] == uiKey);
			OpenViBE::boolean pressedNotOk = (m_mKeyCodes[m_sKeyIncorrect] == uiKey);

			OpenViBE::boolean newSentence = m_vSentences[m_ui32SentenceId].second;

			if (pressedOk == newSentence) m_vStimulationsToSend.push_back(N400S_ANSWER_CORRECT);
			if (pressedNotOk == newSentence) m_vStimulationsToSend.push_back(N400S_ANSWER_INCORRECT);

			m_bProcessingKeys = false;
			m_bNewIteration = true;
		}

		OpenViBE::boolean CN400Sentences24::validKey(guint uiKey)
		{
			return m_mKeyCodes[m_sKeyCorrect] == uiKey || 
				m_mKeyCodes[m_sKeyIncorrect] == uiKey;
		}
	};
};

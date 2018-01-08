#ifndef __OpenViBEPlugins_BoxAlgorithm_N400Sentences13_H__
#define __OpenViBEPlugins_BoxAlgorithm_N400Sentences13_H__

#include "ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>
#include <vector>
#include <map>

#define OVP_ClassId_BoxAlgorithm_N400Sentences13 OpenViBE::CIdentifier(0x18867563, 0x6BDF23E8)
#define OVP_ClassId_BoxAlgorithm_N400Sentences13Desc OpenViBE::CIdentifier(0x214410F8, 0x41F6485B)

namespace OpenViBEPlugins
{
	namespace N400S
	{

		enum N400S_State
		{
			INIT_CUE,
			WORD,
			ANSWER
		};

		class CN400Sentences13 :
				public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			CN400Sentences13(void);

			virtual void release(void) { delete this; }

			// OPENVIBE-RELATED METHODS
			virtual OpenViBE::boolean initialize();
			virtual OpenViBE::boolean uninitialize();
			virtual OpenViBE::uint64 getClockFrequency(void){ return (128LL<<32); }
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			virtual OpenViBE::boolean process(void) { return true; }

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_BoxAlgorithm_N400Sentences13)

			// EXPERIMENT METHODS
			virtual OpenViBE::boolean prepareDataset(OpenViBE::CString experimentDirectory);
			virtual std::vector<::GtkWidget*> createWords(std::string sentence);
			virtual ::GtkWidget* createLabel(std::string text);

			virtual void sendStimulations();

			// callback methods
			virtual void showLabel(::GtkWidget* label);
			virtual void processKey(guint uiKey);
			virtual OpenViBE::boolean validKey(guint uiKey);

		protected:

			// For sending stimulations
			OpenViBEToolkit::TStimulationEncoder<CN400Sentences13> m_oEncoder;

			// Box settings

			OpenViBE::uint64				m_ui64StartingCueDuration;
			OpenViBE::uint64				m_ui64WordDuration;
			OpenViBE::uint64				m_ui64AnswerTimoutDuration;
			OpenViBE::uint32				m_ui32FontSize;
			OpenViBE::uint32				m_ui32NumSentencesPerBlock;
			OpenViBE::CString				m_sKeyCorrect;
			OpenViBE::CString				m_sKeyIncorrect;
			OpenViBE::CString				m_sStartCue;
			OpenViBE::CString				m_sAnswerCue;
			
			// Key codes
			std::map<OpenViBE::CString, OpenViBE::uint32> m_mKeyCodes;
			
			// GTK
			std::vector<std::pair<std::vector<::GtkWidget*>, OpenViBE::uint32>> m_vSentences;
			::GtkWidget*					m_pMainWindow;
			::GtkWidget*					m_pStartCueLabel;
			::GtkWidget*					m_pAnswerCueLabel;
			::GtkWidget*					m_pPrevLabel;

			OpenViBE::uint32				m_ui32SentenceId;
			OpenViBE::uint32				m_ui32WordId;

			OpenViBE::boolean				m_bExperimentStarted; // subject starts experiment by pressing any key
			OpenViBE::boolean				m_bProcessingKeys; // whether inputs should be processed at given time

			// required for sending stimulations
			OpenViBE::uint64				m_ui64PreviousTime; // absolute time
			OpenViBE::uint64				m_ui64CurrentTime; // absolute time

			OpenViBE::uint64				m_ui64StartSentenceTime; // relative time
			OpenViBE::boolean				m_bNewIteration;

			std::vector<OpenViBE::uint64>	m_vStimulationsToSend;

			// current state of running experiment
			N400S_State						m_eCurrentCue;
		};

		/**
		 * Plugin's description
		 */
		class CN400Sentences13Desc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("N400 Sentences 13"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Peter Gergel"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("FMPH UK"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("N400 Sentences 13"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("N400 Sentences 13"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual void release(void)                                   { }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_N400Sentences13; }

			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-fullscreen"); }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::N400S::CN400Sentences13(); }
			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePF) const
			{
				return ePF == OpenViBE::Kernel::PluginFunctionality_Visualization;
			}

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addSetting("Start of sentence cue duration in ms", OV_TypeId_Integer, "2500");
				rPrototype.addSetting("Word duration in ms", OV_TypeId_Integer, "500");
				rPrototype.addSetting("Answer timeout in ms", OV_TypeId_Integer, "2500");
				rPrototype.addSetting("Key correct", OV_TypeId_String, "1");
				rPrototype.addSetting("Key incorrect", OV_TypeId_String, "3");
				rPrototype.addSetting("N400-sentences directory", OV_TypeId_String, "c:/Users/pege/Downloads/openvibe-1.1.0-src/n400-sentences");
				rPrototype.addSetting("Senteces dataset", OV_TypeId_String, "dataset.tsv");
				rPrototype.addSetting("Font size", OV_TypeId_Integer, "16");
				rPrototype.addSetting("Num. sentences per group", OV_TypeId_Integer, "20");
				rPrototype.addSetting("Start of the sentence cue", OV_TypeId_String, "+");
				rPrototype.addSetting("Answer request cue", OV_TypeId_String, "?");

				rPrototype.addOutput("N400-sentences stimulations", OV_TypeId_Stimulations);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_N400Sentences13Desc)
		};
	};
};

#endif
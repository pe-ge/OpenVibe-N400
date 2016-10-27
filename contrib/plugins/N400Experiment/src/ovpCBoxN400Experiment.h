#ifndef __OpenViBEPlugins_BoxAlgorithm_N400Experiment_H__
#define __OpenViBEPlugins_BoxAlgorithm_N400Experiment_H__

#include "ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>
#include <vector>
#include <map>

#define OVP_ClassId_BoxAlgorithm_N400Experiment OpenViBE::CIdentifier(0x1F480B76, 0x2C9B52CF)
#define OVP_ClassId_BoxAlgorithm_N400ExperimentDesc OpenViBE::CIdentifier(0x533A0915, 0x519E4B68)

namespace OpenViBEPlugins
{
	namespace N400
	{

		enum N400Cue
		{
			CROSS		= 0,
			PAUSE1		= 1,
			PICTURE1	= 2,
			PAUSE2		= 3,
			PICTURE2	= 4,
			PAUSE3		= 5,
			ANSWER		= 6,
			TOTAL_CUES	= 7
		};

		class CN400Experiment :
				public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			CN400Experiment(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize();
			virtual OpenViBE::boolean uninitialize();
			virtual OpenViBE::uint64 getClockFrequency(void){ return (128LL<<32); }
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			virtual OpenViBE::boolean process(void) { return true; }

			virtual OpenViBE::boolean loadDataset(OpenViBE::CString experimentDirectory);
			virtual void createMatches();

			virtual void sendStimulations();

			// callback methods
			virtual void redraw(void);
			virtual void processKey(guint uiKey);
			virtual OpenViBE::boolean validKey(guint uiKey);

			// drawing
			virtual void drawPicture(OpenViBE::uint32 uint32CueID);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_BoxAlgorithm_N400Experiment)

		protected:

			// For sending stimulations
			OpenViBEToolkit::TStimulationEncoder<CN400Experiment> m_oEncoder;

			// Box settings
			OpenViBE::uint32	m_ui32PictureWidth;
			OpenViBE::uint32	m_ui32PictureHeight;

			OpenViBE::uint64	m_ui64CrossDuration;
			OpenViBE::uint64	m_ui64PictureDuration;
			OpenViBE::uint64	m_ui64FirstPauseDuration;
			OpenViBE::uint64	m_ui64SecondPauseDuration;
			OpenViBE::uint64	m_ui64ThirdPauseDuration;

			::GtkWidget*		m_pMainWindow;

			OpenViBE::uint32	m_ui32RequestedPictureID;
			OpenViBE::boolean	m_bNewIteration;
			N400Cue				m_eCurrentCue;

			std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>> m_vImagesDataset;
			std::vector<OpenViBE::boolean>	m_vMatches;

			OpenViBE::uint64	m_ui64PreviousTime;
			OpenViBE::uint64	m_ui64CurrentTime;
			OpenViBE::uint64	m_ui64NewIterationTime;

			// Buttons
			std::map<OpenViBE::CString, OpenViBE::uint32> m_mButtonCodes;
			OpenViBE::CString	m_sMatchingButton;
			OpenViBE::CString	m_sNonmatchingButton;
			OpenViBE::CString	m_sUnsureButton;

			OpenViBE::boolean	m_bProcessingKeys;

			OpenViBE::boolean	m_bExperimentStarted;

			std::vector<OpenViBE::uint64> m_vStimulationsToSend;
		};

		/**
		 * Plugin's description
		 */
		class CN400ExperimentDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("N400 Experiment"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Peter Gergel"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("FMPH UK"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("N400 Experiment"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("N400 Experimentc"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual void release(void)                                   { }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_N400Experiment; }

			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-fullscreen"); }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::N400::CN400Experiment(); }
			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePF) const
			{
				return ePF == OpenViBE::Kernel::PluginFunctionality_Visualization;
			}

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addSetting("Picture width", OV_TypeId_Integer, "600");
				rPrototype.addSetting("Picture height", OV_TypeId_Integer, "600");
				rPrototype.addSetting("Cross duration in ms", OV_TypeId_Integer, "200");
				rPrototype.addSetting("Picture duration in ms", OV_TypeId_Integer, "3000");
				rPrototype.addSetting("First pause duration in ms", OV_TypeId_Integer, "500");
				rPrototype.addSetting("Second pause duration in ms", OV_TypeId_Integer, "2000");
				rPrototype.addSetting("Third pause duration in ms", OV_TypeId_Integer, "500");
				rPrototype.addSetting("Matching button", OV_TypeId_String, "1");
				rPrototype.addSetting("Nonmatching button", OV_TypeId_String, "3");
				rPrototype.addSetting("Unsure button", OV_TypeId_String, "2");
				rPrototype.addSetting("Experiment directory", OV_TypeId_String, "experiment1");
				rPrototype.addSetting("Experiment iteration", OV_TypeId_String, "1");

				rPrototype.addOutput("N400 related stimulations", OV_TypeId_Stimulations);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_N400ExperimentDesc)
		};
	};
};

#endif

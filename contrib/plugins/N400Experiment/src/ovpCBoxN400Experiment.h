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
			PICTURE1	= 1,
			PAUSE1		= 2,
			PICTURE2	= 3,
			PAUSE2		= 4,
			ANSWER		= 5,
			TOTAL_CUES	= 6
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

			// stimulation-related methods
			virtual void sendStimulation(OpenViBE::uint64 ui64StimulationIdentifier, OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime);
			virtual void sendCurrentCue(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime);
			virtual void sendPressedButton(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime);
			virtual void sendBeep(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime);

			// callback methods
			virtual void redraw(void);
			virtual void resize(OpenViBE::uint32 ui32Width, OpenViBE::uint32 ui32Height);
			virtual void processKey(guint uiKey);
			virtual OpenViBE::boolean validKey(guint uiKey);

			virtual void drawCuePicture(OpenViBE::uint32 uint32CueID);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_BoxAlgorithm_N400Experiment)

		protected:

			// For sending stimulations
			OpenViBEToolkit::TStimulationEncoder<CN400Experiment> m_oEncoder;

			// Box settings
			OpenViBE::uint64 m_ui64CrossDuration;
			OpenViBE::uint64 m_ui64PictureDuration;
			OpenViBE::uint64 m_ui64PauseDuration;

			// The Builder handler used to create the interface
			::GtkBuilder* m_pBuilderInterface;
			::GtkWidget*  m_pMainWindow;
			::GtkWidget*  m_pDrawingArea;

			OpenViBE::uint32	m_ui32RequestedPictureID;
			OpenViBE::boolean	m_bRequestDraw;
			OpenViBE::boolean	m_bNewIteration;
			N400Cue				m_eCurrentCue;

			std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>> m_pOriginalPicture;
			std::vector<std::pair<OpenViBE::CString, ::GdkPixbuf*>> m_pScaledPicture;

			::GdkColor m_oBackgroundColor;
			::GdkColor m_oForegroundColor;

			OpenViBE::uint64	m_ui64PreviousActivationTime;
			OpenViBE::uint64	m_ui64NewIterationTime;

			// Buttons
			std::map<OpenViBE::CString, OpenViBE::uint32> m_mButtonCodes;
			OpenViBE::CString	m_sRightButton;
			OpenViBE::CString	m_sWrongButton;
			OpenViBE::CString	m_sUnsureButton;

			OpenViBE::boolean	m_bProcessingKeys;
			OpenViBE::uint32	m_ui32PressedButton;
			OpenViBE::boolean	m_bRequestProcessButton;

			OpenViBE::boolean	m_bRequestBeep;

			OpenViBE::boolean	m_bExperimentStarted;
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
				rPrototype.addSetting("Window width", OV_TypeId_Integer, "600");
				rPrototype.addSetting("Window height", OV_TypeId_Integer, "600");
				rPrototype.addSetting("Cross duration in ms", OV_TypeId_Integer, "200");
				rPrototype.addSetting("Picture duration in ms", OV_TypeId_Integer, "3000");
				rPrototype.addSetting("Pause duration in ms", OV_TypeId_Integer, "2000");
				rPrototype.addSetting("Right answer button", OV_TypeId_String, "1");
				rPrototype.addSetting("Wrong answer button", OV_TypeId_String, "3");
				rPrototype.addSetting("Unsure button", OV_TypeId_String, "2");

				rPrototype.addOutput("Time of appearance of visual cue", OV_TypeId_Stimulations);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_N400ExperimentDesc)
		};
	};
};

#endif

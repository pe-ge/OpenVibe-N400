#include "ovpCBoxDisplayDynamicCueImage.h"
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		gboolean DisplayDynamicCueImage_SizeAllocateCallback(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
		{
			reinterpret_cast<CDisplayDynamicCueImage*>(data)->resize((uint32)allocation->width, (uint32)allocation->height);
			return FALSE;
		}

		gboolean DisplayDynamicCueImage_RedrawCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
		{
			reinterpret_cast<CDisplayDynamicCueImage*>(data)->redraw();
			return TRUE;
		}

		CDisplayDynamicCueImage::CDisplayDynamicCueImage(void) :
			m_pBuilderInterface(NULL),
			m_pMainWindow(NULL),
			m_pDrawingArea(NULL),
			//m_pStimulationReaderCallBack(NULL),
			m_bImageRequested(false),
			m_bImageDrawn(false),
			m_pOriginalPicture(NULL),
			m_pScaledPicture(NULL)
		{
			m_oBackgroundColor.pixel = 0;
			m_oBackgroundColor.red = 0;
			m_oBackgroundColor.green = 0;
			m_oBackgroundColor.blue = 0;

			m_oForegroundColor.pixel = 0;
			m_oForegroundColor.red = 0xFFFF;
			m_oForegroundColor.green = 0xFFFF;
			m_oForegroundColor.blue = 0xFFFF;
		}

		boolean CDisplayDynamicCueImage::initialize()
		{
			//>>>> Reading Settings:

			//Number of Cues:
			CString l_sSettingValue;

			//Do we display the images in full screen?
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, l_sSettingValue);

			//Clear screen stimulation:
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, l_sSettingValue);

			//Stimulation ID and images file names for each cue

			//load the gtk builder interface
			m_pBuilderInterface=gtk_builder_new();
			gtk_builder_add_from_file(m_pBuilderInterface, OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-DisplayCueImage.ui", NULL);

			if(!m_pBuilderInterface)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Couldn't load the interface !";
				return false;
			}

			gtk_builder_connect_signals(m_pBuilderInterface, NULL);

			m_pDrawingArea = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "DisplayCueImageDrawingArea"));
			g_signal_connect(G_OBJECT(m_pDrawingArea), "expose_event", G_CALLBACK(DisplayDynamicCueImage_RedrawCallback), this);
			g_signal_connect(G_OBJECT(m_pDrawingArea), "size-allocate", G_CALLBACK(DisplayDynamicCueImage_SizeAllocateCallback), this);

			//set widget bg color
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oBackgroundColor);

			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oForegroundColor);

			//Load the pictures:

			std::cout << OpenViBE::Directories::getDataDir() << std::endl;

			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);

			return true;
		}

		boolean CDisplayDynamicCueImage::uninitialize()
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

			return true;
		}

		boolean CDisplayDynamicCueImage::processClock(CMessageClock& rMessageClock)
		{
			IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();
			return true;
		}

		boolean CDisplayDynamicCueImage::processInput(uint32 ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		boolean CDisplayDynamicCueImage::process()
		{
			return true;
		}

		//Callback called by GTK
		void CDisplayDynamicCueImage::redraw()
		{

		}

		void CDisplayDynamicCueImage::drawCuePicture(OpenViBE::uint32 uint32CueID)
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

				gint l_iX = (l_iWindowWidth/2) - gdk_pixbuf_get_width(m_pScaledPicture[uint32CueID])/2;
				gint l_iY = (l_iWindowHeight/2) - gdk_pixbuf_get_height(m_pScaledPicture[uint32CueID])/2;;
				gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pScaledPicture[uint32CueID], 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}

		void CDisplayDynamicCueImage::resize(uint32 ui32Width, uint32 ui32Height)
		{
			
		}
	};
};

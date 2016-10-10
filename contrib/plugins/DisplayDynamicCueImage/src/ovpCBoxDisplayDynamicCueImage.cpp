#include "ovpCBoxDisplayDynamicCueImage.h"

/* TODO:
 *	using regexes in function filenamesCompare
 *	 
*/

#include "boost/filesystem.hpp"
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

using namespace boost::filesystem;

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

		CDisplayDynamicCueImage::CDisplayDynamicCueImage(void) :
			m_ui64CrossDuration(0),
			m_ui64PictureDuration(0),
			m_ui64PauseDuration(0),
			m_ui64TotalIterations(0),
			m_pBuilderInterface(NULL),
			m_pMainWindow(NULL),
			m_pDrawingArea(NULL),
			m_ui32NumberOfCue(0),
			m_ui32RequestedPictureID(1),
			m_bRequestDraw(false), // for initial cross
			m_eCurrentCue(CROSS),
			m_ui64PreviousActivationTime(0),
			m_ui32IterationCount(0),
			m_ui64ExperimentDuration(0)
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

		OpenViBE::boolean CDisplayDynamicCueImage::initialize()
		{
			//>>>> Reading Settings:

			// Image directory path
			CString l_sDirectoryPath;
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, l_sDirectoryPath);

			// Image extension
			CString l_sImageExtension;
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, l_sImageExtension);

			// Durations
			m_ui64CrossDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext( ), 2);
			m_ui64PictureDuration	= FSettingValueAutoCast(*this->getBoxAlgorithmContext( ), 3);
			m_ui64PauseDuration		= FSettingValueAutoCast(*this->getBoxAlgorithmContext( ), 4);

			m_ui64ExperimentDuration = m_ui64CrossDuration + 2 * m_ui64PictureDuration + 2 * m_ui64PauseDuration;

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

			m_ui32NumberOfCue = m_pOriginalPicture.size();

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

			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);

			// output stimulation
			m_oEncoder.initialize(*this,0);
			m_oEncoder.encodeHeader();
			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
			//sendCurrentCue(0, 0);

			return true;
		}

		OpenViBE::boolean CDisplayDynamicCueImage::uninitialize()
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

			for (uint32 i = 0; i < m_ui32NumberOfCue; i++) {
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

		OpenViBE::boolean CDisplayDynamicCueImage::processClock(CMessageClock& rMessageClock)
		{
			// Static variables
			static uint32 l_ui64FirstPictureTime = m_ui64CrossDuration;
			static uint32 l_ui64FirstPauseTime = l_ui64FirstPictureTime + m_ui64PictureDuration;
			static uint32 l_ui64SecondPictureTime = l_ui64FirstPauseTime + m_ui64PauseDuration;
			static uint32 l_ui64SecondPauseTime = l_ui64SecondPictureTime + m_ui64PictureDuration;

			const uint64 l_ui64CurrentTime=rMessageClock.getTime();

			// start of new iteration?
			if (l_ui64CurrentTime < m_ui64PreviousActivationTime) {
				m_ui32IterationCount++;
				m_eCurrentCue = CROSS;
				m_bRequestDraw = true;
				std::cout << "cross" << std::endl;
			}

			if (!m_bRequestDraw) {
				// obtain time of current iteration in ms
				uint64 l_ui64CurrentIterationTime = (uint64)((l_ui64CurrentTime >> 16) / 65.5360) % m_ui64ExperimentDuration;

				// First picture
				if ((m_eCurrentCue == PICTURE1) && (l_ui64CurrentIterationTime >= l_ui64FirstPictureTime)) {
					m_bRequestDraw = true;
					std::cout << "1picture" << std::endl;
				}
				// First pause
				else if ((m_eCurrentCue == PAUSE1) && (l_ui64CurrentIterationTime >= l_ui64FirstPauseTime)) {
					m_bRequestDraw = true;
					std::cout << "1pause" << std::endl;
				}
				// Second picture
				else if ((m_eCurrentCue == PICTURE2) && (l_ui64CurrentIterationTime >= l_ui64SecondPictureTime)) {
					m_bRequestDraw = true;
					std::cout << "2pic" << std::endl;
				}
				// Second pause
				else if ((m_eCurrentCue == PAUSE2) && (l_ui64CurrentIterationTime >= l_ui64SecondPauseTime)) {
					m_bRequestDraw = true;					
					std::cout << "2pause" << std::endl;
				}
			}
			
			if(m_bRequestDraw && GTK_WIDGET(m_pDrawingArea)->window)
			{
				gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,NULL,true);
				sendCurrentCue(m_ui64PreviousActivationTime, l_ui64CurrentTime);
			}

			m_ui64PreviousActivationTime = l_ui64CurrentTime;

			return true;
		}

		void CDisplayDynamicCueImage::sendCurrentCue(OpenViBE::uint64 ui64PreviousTime, OpenViBE::uint64 ui64CurrentTime)
		{
			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
			IStimulationSet* l_pStimulationSet = m_oEncoder.getInputStimulationSet();
			l_pStimulationSet->clear();		// The encoder may retain the buffer from the previous round, clear it
			l_pStimulationSet->appendStimulation(m_eCurrentCue, ui64CurrentTime, 0);
			m_oEncoder.encodeBuffer();
			l_pBoxIO->markOutputAsReadyToSend(0, ui64PreviousTime, ui64CurrentTime);
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
		}

		//Callback called by GTK
		void CDisplayDynamicCueImage::redraw()
		{
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
			m_eCurrentCue = N400Cue((m_eCurrentCue + 1) % 5);
			m_bRequestDraw = false;
		}

		void CDisplayDynamicCueImage::drawCuePicture(OpenViBE::uint32 uint32CueID)
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;
			gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pScaledPicture[uint32CueID].second, 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}

		void CDisplayDynamicCueImage::resize(uint32 ui32Width, uint32 ui32Height)
		{
			for (uint32 i = 0; i < m_ui32NumberOfCue; i++) {
				if (m_pScaledPicture[i].second) {
					g_object_unref(G_OBJECT(m_pScaledPicture[i].second));
				}
				m_pScaledPicture[i].second = gdk_pixbuf_scale_simple(m_pOriginalPicture[i].second, ui32Width, ui32Height, GDK_INTERP_BILINEAR);
			}
		}
	};
};

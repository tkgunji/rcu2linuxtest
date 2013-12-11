/*
 * $Id: mainpage.c,v 1.2 2006/05/26 12:18:44 richter Exp $
 *
/************************************************************************
**
** RCU ARM linux tools and drivers
** Copyright (c) 2006
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
** Matthias.Richter@ift.uib.no
**
** Permission to use, copy, modify and distribute this software and its  
** documentation strictly for non-commercial purposes is hereby granted  
** without fee, provided that the above copyright notice appears in all  
** copies and that both the copyright notice and this permission notice  
** appear in the supporting documentation. The authors make no claims    
** about the suitability of this software for any purpose. It is         
** provided "as is" without express or implied warranty.                 
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free
** Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
** MA 02111-1307  USA  
**
*************************************************************************/

/** @file   mainpage.c
    @author Matthias Richter
    @date   
    @brief  Title page documentation. */
/** @mainpage RCU linux

    @section intro Introduction

    @section overview Overview

    <center><img src="pic_DSCboard-tools-schematic.gif" border="0" alt=""></center>
    
    @section sysregs System requirements
    The package can be built on Linux systems. In order to test the specific hardware
    access a DCS board is necessary. The ARM linux runs in an embedded environment, to
    build applications for it one needs a <i>Cross Compiler</i>.

    @section sw_components The S/W Components
    - The @ref msg_buffer_interface specifies a memory mapped interface between the
    DCS board and the RCU motherboard. The @ref dcsc_msg_buffer_access module does the
    job of encoding data buffers in the specific format.
    - The @ref rcu_sh is a low level shell-like tool to access the RCU memory space.
    - The @ref sm_tools module hosts tools to access the <i>SelectMap(tm)</i> interface
    of the Xilinx FPGA.

    @section drivers The drivers
    - @ref rcubus_driver: The driver provides the access to memory/registers inside
    the DCS board firmware. Althoug it was originally written to feature the
    @ref msg_buffer_interface and the communication between the DCS board and the 
    RCU motherboard, it does not contain any RCU specific code. The driver can be used
    to access three memory regions of configurable location and size inside the
    firmware address space.

    @section mpg_links Related links on the web

    
    - <a class="el" href="http://www.ift.uib.no/~kjeks/wiki/index.php?title=Detector_Control_System_%28DCS%29_for_ALICE_Front-end_electronics">
          Detector Control System for the ALICE TPC electronics </a> 
    - <a class="el" href="http://ep-ed-alice-tpc.web.cern.ch/ep-ed-alice-tpc/">ALICE TPC electronics pages</a>
    - <a class="el" href="http://www.kip.uni-heidelberg.de/ti/DCS-Board/current/"> DCS board pages</a>
    - <a class="el" href="http://www."> DCS board linux pages</a>
    - <a class="el" href="http://www."> uClinux pages</a>
    - <a class="el" href="http://alicedcs.web.cern.ch/AliceDCS/"> ALICE DCS pages</a>
    - <a class="el" href="http://www.ztt.fh-worms.de/en/projects/Alice-FEEControl/index.shtml"> 
          FeeCom Software pages (ZTT Worms)</a>


*/

#error Not for compilation
//
// EOF
//

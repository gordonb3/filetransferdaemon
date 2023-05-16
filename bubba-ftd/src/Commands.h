/*
    
    DownloadManager - http://www.excito.com/
    
    Commands.h - this file is part of DownloadManager.
    
    Copyright (C) 2007 Tor Krill <tor@excito.com>
    
    DownloadManager is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    DownloadManager is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id: Commands.h 2011 2008-10-13 21:05:02Z tor $
*/
#ifndef MY_COMMANDS_H
#define MY_COMMANDS_H

typedef enum my_cmd_type{
    CMD_OK,					/* Command was successful */
    CMD_FAIL,				/* Command falied */
    ADD_DOWNLOAD,			/* Add download for user user, with uuid, url and policy*/ 
    CANCEL_DOWNLOAD,		/* Cancel download url for user */
    LIST_DOWNLOADS,			/* List downloads by user or user and uuid */
    GET_UPLOAD_THROTTLE,	/* Retreive upload throttle for service in name returns size */
    SET_UPLOAD_THROTTLE,	/* Set upload throttle, name is service and size is amt */
    GET_DOWNLOAD_THROTTLE,	/* Retreive download throttle for service in name returns size */
    SET_DOWNLOAD_THROTTLE,	/* Set download throttle, name is service and size is amt */
    CMD_SHUTDOWN
} cmd_type;

/* Policies for download */

#define DLP_NONE		0x0000
#define DLP_AUTOREMOVE	0x0001
#define DLP_HIDDEN		0x0002

#define CMD_USERSIZE	32
#define CMD_URLSIZE		200
#define CMD_NAMESIZE	100
#define CMD_INFOSIZE	200
#define CMD_UUIDSIZE	20

#endif

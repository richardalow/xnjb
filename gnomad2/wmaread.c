/* wmaread.c
   WMA interface
   Copyright (C) 2004 Linus Walleij

This file is part of the GNOMAD package.

GNOMAD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

You should have received a copy of the GNU General Public License
along with GNOMAD; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA. 

Much of the code in this file was derived from the getid3() code,
written in PHP. The C implementation here is however made from
scratch.

This file has been modified by Richard Low to use in XNJB.

*/

#include "wmaread.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "wma_unicode.h"

gboolean guid_compare(guchar *guid1, guchar *guid2);
void get_ucs2_string(const guchar *in, guint length, gchar **out);
void parse_content_description(guchar *data, guint size, metadata_t *meta);
void parse_extended_content_description(guchar *data, guint size, metadata_t *meta);
void parse_file_properties(guchar *data, guint size, metadata_t *meta);
void parse_header_extension(guchar *data, guint size, metadata_t *meta);
guint string_to_guint(gchar *string);

/* Converts a string into guint representation (if possible) */
guint string_to_guint(gchar *string)
{
  gchar *dummy;
	
  if (!string)
    return 0;
  
  return (guint) strtoul(string, &dummy, 10);
}

gboolean guid_compare(guchar *guid1, guchar *guid2)
{
  gboolean retval = TRUE;
  guint i;

  for (i = 0; i < 16; i++)
	{
    if (guid1[i] != guid2[i])
		{
      retval = FALSE;
      break;
    }
  }
  return retval;
}

void get_ucs2_string(const guchar *in, guint length, gchar **out)
{
	// should all be even lengths, warn if not
	if (length % 2 != 0)
		printf("Length is not even in get_ucs2_string!");
	gunichar2 *in2 = malloc(length);
	int i = 0;
	for (i = 0; i < (length/2); i+=1)
	{
		// the data coming in is little endian, make sure it works
		// on big endian platforms
		in2[i] = in[2*i] + (in[(2*i)+1] << 8);
	}
	*out = g_utf16_to_utf8((const gunichar2 *)in2, length, NULL, NULL);
	free(in2);
}

void parse_content_description(guchar *data, guint size, metadata_t *meta)
{
  // This extracts some metadata from the content description object
  // Content Description Object: (optional, one only)
  // Field Name           Field Type  Size (bits)
  // Object ID            GUID        128     // GUID for Content Description object - GETID3_ASF_Content_Description_Object
  // Object Size          QWORD       64      // size of Content Description object, including 34 bytes of Content Description Object header
  // Title Length         WORD        16      // number of bytes in Title field
  // Author Length        WORD        16      // number of bytes in Author field
  // Copyright Length     WORD        16      // number of bytes in Copyright field
  // Description Length   WORD        16      // number of bytes in Description field
  // Rating Length        WORD        16      // number of bytes in Rating field
  // Title                WCHAR       16      // array of Unicode characters - Title
  // Author               WCHAR       16      // array of Unicode characters - Author
  // Copyright            WCHAR       16      // array of Unicode characters - Copyright
  // Description          WCHAR       16      // array of Unicode characters - Description
  // Rating               WCHAR       16      // array of Unicode characters - Rating
  guint title_length = (data[25] << 8) + data[24];
  guint author_length = (data[27] << 8) + data[26];
  /* unused
	guint copyright_length = (data[29] << 8) + data[28];
  guint description_length = (data[31] << 8) + data[30];
  guint rating_length = (data[33] << 8) + data[32];
	*/
  guint offset = 34; // Points to first string

  if ((title_length > 0) && (meta->title == NULL))
	{
    get_ucs2_string(&data[offset], title_length, &meta->title);
    //printf("Found title: %s\n", meta->title);
  }
  offset += title_length;
  if ((offset < size) && (author_length > 0) && (meta->artist == NULL))
	{
    get_ucs2_string(&data[offset], author_length, &meta->artist);
    //printf("Found author/artist: %s\n", meta->artist);
  }
  // Skip the other fields.
  offset += author_length;
  return;
}

void parse_extended_content_description(guchar *data, guint size, metadata_t *meta)
{
  // Extended Content Description Object: (optional, one only)
  // Field Name                   Field Type   Size (bits) Offset
  // Object ID                    GUID         128         0    // GUID for Extended Content Description object - GETID3_ASF_Extended_Content_Description_Object
  // Object Size                  QWORD        64          16   // size of ExtendedContent Description object, including 26 bytes of Extended Content Description Object header
  // Content Descriptors Count    WORD         16          24   // number of entries in Content Descriptors list
  // Content Descriptors          array of:    variable    26   //
  // * Descriptor Name Length     WORD         16               // size in bytes of Descriptor Name field
  // * Descriptor Name            WCHAR        variable         // array of Unicode characters - Descriptor Name
  // * Descriptor Value Data Type WORD         16               // Lookup array:
  // 0x0000 = Unicode String (variable length)
  // 0x0001 = BYTE array     (variable length)
  // 0x0002 = BOOL           (DWORD, 32 bits)
  // 0x0003 = DWORD          (DWORD, 32 bits)
  // 0x0004 = QWORD          (QWORD, 64 bits)
  // 0x0005 = WORD           (WORD,  16 bits)
  // * Descriptor Value Length    WORD         16              // number of bytes stored in Descriptor Value field
  // * Descriptor Value           variable     variable        // value for Content Descriptor
  guint descriptors = (data[25] << 8) + data[24];
  guint offset = 26;
  //printf("%d descriptors\n", descriptors);
	
	// did we get a WM/TrackNumber? If so, don't read WM/Track
	gboolean gotTrackNumber = FALSE;
  while ((descriptors > 0) && (offset < size))
	{
    guint desc_namelen;
    gchar *desc_name;
    guint desc_datatype;
    guint desc_datalen;

    desc_namelen = (data[offset+1] << 8) + data[offset];
    offset += 2;
    get_ucs2_string(&data[offset], desc_namelen, &desc_name);
    offset += desc_namelen;
    desc_datatype = (data[offset+1] << 8) + data[offset];
    offset += 2;
    desc_datalen = (data[offset+1] << 8) + data[offset];
    offset += 2;
    if (desc_datatype == 0x0000)
		{
      // Unicode string. Let's look at it...
      gchar *desc_string = NULL;
      get_ucs2_string(&data[offset], desc_datalen, &desc_string);
      //printf("Descriptor: %s, value=\'%s\'\n", desc_name, desc_string);
      if (!strcmp(desc_name, "WM/Genre") || !strcmp(desc_name, "Genre"))
			{
				if (meta->genre != NULL)
					free(meta->genre);
				meta->genre = desc_string;
			}
			else if(!strcmp(desc_name, "WM/AlbumArtist") || !strcmp(desc_name, "Artist"))
			{
				if (meta->artist != NULL)
					free(meta->artist);
				meta->artist = desc_string;
			}
			else if(!strcmp(desc_name, "WM/AlbumTitle") || !strcmp(desc_name, "Album"))
			{
				if (meta->album != NULL)
					free(meta->album);
				meta->album = desc_string;
			}
			else if(!strcmp(desc_name, "WM/Year") || !strcmp(desc_name, "Year"))
			{
				meta->year = string_to_guint(desc_string);
				free(desc_string);
			}
			else if(!strcmp(desc_name, "WM/TrackNumber") || !strcmp(desc_name, "TrackNumber"))
			{
				meta->trackno = string_to_guint(desc_string);
				free(desc_string);
				gotTrackNumber = TRUE;
			}
			else if(!gotTrackNumber && !strcmp(desc_name, "WM/Track") || !strcmp(desc_name, "Track"))
			{
				meta->trackno = string_to_guint(desc_string);
				free(desc_string);
			}
			else
			{
				free(desc_string);
			}
		}
		else if (desc_datatype == 0x0003)
		{
			guint desc_value = (data[offset+3] << 24) + (data[offset+2] << 16) + (data[offset+1] << 8) + data[offset];
				
			//printf("Skipped descriptor: %s, value=\'%lu\'\n", desc_name, desc_value);
			if (!strcmp(desc_name, "WM/TrackNumber") || !strcmp(desc_name, "TrackNumber"))
			{
				meta->trackno = desc_value;
				gotTrackNumber = TRUE;
			}
			else if (!gotTrackNumber && !strcmp(desc_name, "WM/Track") || !strcmp(desc_name, "Track"))
			{
				meta->trackno = desc_value;
			}
		}
		//else
		//{
		//	printf("Skipped descriptor: %s (type=%d,namelen=%lu,datalen=%lu)\n", desc_name, desc_datatype, desc_namelen, desc_datalen);
		//}
		offset += desc_datalen;
    free(desc_name);
    descriptors--;
  }
}

void parse_file_properties(guchar *data, guint size, metadata_t *meta)
{
  // File Properties Object: (mandatory, one only)
  // Field Name                Field Type   Size (bits) Offset
  // Object ID                 GUID         128         0   // GUID for file properties object - GETID3_ASF_File_Properties_Object
  // Object Size               QWORD        64          16  // size of file properties object, including 104 bytes of File Properties Object header
  // File ID                   GUID         128         24  // unique ID - identical to File ID in Data Object
  // File Size                 QWORD        64          40  // entire file in bytes. Invalid if Broadcast Flag == 1
  // Creation Date             QWORD        64          48  // date & time of file creation. Maybe invalid if Broadcast Flag == 1
  // Data Packets Count        QWORD        64          56  // number of data packets in Data Object. Invalid if Broadcast Flag == 1
  // Play Duration             QWORD        64          64  // playtime, in 100-nanosecond units. Invalid if Broadcast Flag == 1
  // Send Duration             QWORD        64          72  // time needed to send file, in 100-nanosecond units. Players can ignore this value. Invalid if Broadcast Flag == 1
  // Preroll                   QWORD        64          80  // time to buffer data before starting to play file, in 1-millisecond units. If <> 0, PlayDuration and PresentationTime have been offset by this amount
  // Flags                     DWORD        32          88  //
  // * Broadcast Flag          bits         1  (0x01)       // file is currently being written, some header values are invalid
  // * Seekable Flag           bits         1  (0x02)       // is file seekable
  // * Reserved                bits         30 (0xFFFFFFFC) // reserved - set to zero
  // Minimum Data Packet Size  DWORD        32          92  // in bytes. should be same as Maximum Data Packet Size. Invalid if Broadcast Flag == 1
  // Maximum Data Packet Size  DWORD        32          96  // in bytes. should be same as Minimum Data Packet Size. Invalid if Broadcast Flag == 1
  // Maximum Bitrate           DWORD        32          100 // maximum instantaneous bitrate in bits per second for entire file, including all data streams and ASF overhead
  guint flags = (data[89] << 8) + data[88];
  guint64 playtime = ((guint64) data[71] << 56) + ((guint64) data[70] << 48) + ((guint64) data[69] << 40) + 
    ((guint64) data[68] << 32) + ((guint64) data[67] << 24) + ((guint64) data[66] << 16) + ((guint64) data[65] << 8) + data[64];

  //printf("Flags value %02X, (%d)\n", flags, flags);
  //printf("Playtime: %llu\n", playtime);
  if ((flags & 0x0001) == 0)
	{
    // We have valid values
    gint seconds;

    playtime /= 10000000;
    seconds = (gint) playtime;
    //printf("Play time %d seconds\n", seconds);
    meta->length = seconds;
  }
	else
	{
    printf("Stream had broadcast flag set: length tag is invalid.\n");
  }
  return;
}

void parse_header_extension(guchar *data, guint size, metadata_t *meta)
{
  // Header Extension Object: (mandatory, one only)
  // Field Name                   Field Type   Size (bits) Offset
  // Object ID                    GUID         128         0    // GUID for Header Extension object - GETID3_ASF_Header_Extension_Object
  // Object Size                  QWORD        64          16   // size of Header Extension object, including 46 bytes of Header Extension Object header
  // Reserved Field 1             GUID         128         24   // hardcoded: GETID3_ASF_Reserved_1
  // Reserved Field 2             WORD         16          40   // hardcoded: 0x00000006
  // Header Extension Data Size   DWORD        32          42   // in bytes. valid: 0, or > 24. equals object size minus 46
  // Header Extension Data        BYTESTREAM   variable    46   // array of zero or more extended header objects

  // Not parsed or interpreted in any way.
  return;
}

/* -------------------------------------- */
/* EXPORTED FUNCTIONS                     */
/* -------------------------------------- */

void get_tag_for_wmafile(metadata_t *meta)
{
  gint fd;
  guchar header[30];
  // ASF structure:
  // * Header Object [required]
  //   * File Properties Object [required]   (global file attributes)
  //   * Stream Properties Object [required] (defines media stream & characteristics)
  //   * Header Extension Object [required]  (additional functionality)
  //   * Content Description Object          (bibliographic information)
  //   * Script Command Object               (commands for during playback)
  //   * Marker Object                       (named jumped points within the file)
  // * Data Object [required]
  //   * Data Packets
  // * Index Object
  
  // Header Object: (mandatory, one only)
  // Field Name                   Field Type   Size (bits)
  // Object ID                    GUID         128             // GUID for header object - GETID3_ASF_Header_Object
  // Object Size                  QWORD        64              // size of header object, including 30 bytes of Header Object header
  // Number of Header Objects     DWORD        32              // number of objects in header object
  // Reserved1                    BYTE         8               // hardcoded: 0x01
  // Reserved2                    BYTE         8               // hardcoded: 0x02
  gint n;

//  printf("Getting WMA tag info for %s...\n", meta->path);
  fd = (gint) open(meta->path, O_RDONLY, 0);
  if (fd < 0)
    return;

//  printf("Opened file\n");

  // Read in some stuff...
  n = read(fd,header,30);
  if (n == 30)
	{
    // Hardcoded ASF header GUID
    gchar ASF_header[] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66 ,0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};

    if (guid_compare(ASF_header, header))
		{
      guint headersize;
      guchar *headerobject;
      guint objects;

      if (header[20] != 0x00 ||
					header[21] != 0x00 ||
					header[22] != 0x00 ||
					header[23] != 0x00)
			{
				// We don't handle tags which require this magnitude of memory to read in!
				return;
			}
      headersize = (header[19] << 24) + (header[18] << 16) + (header[17] << 8) + header[16] - 30;
      objects = (header[27] << 24) + (header[26] << 16) + (header[25] << 8) + header[24];
      //printf("Found an ASF header of %lu bytes with %lu objects\n", headersize, objects);
      headerobject = malloc(headersize);
      n = read(fd,headerobject,headersize);
      if (n == headersize)
			{
				// Parse the header...
				guchar asf_object_guid[16];
				guint asf_object_size;
				guint current_object = 0;
				guint offset = 0;

				//printf("Retrieved ASF header\n");
				while (current_object < objects)
				{
					// Available objects
					// GUID entrance pattern:
					// 8CABDCA1-A947-11CF-8EE4-00C00C205365 ->
					// A1DCAB8C 47A9 CF11 8EE4 00C00C205365
	  
					guchar File_Properties_Object_GUID[] = 
						{0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
					guchar Stream_Properties_Object_GUID[] =
						{0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
					guchar Header_Extension_Object_GUID[] =
						{0xB5, 0x03, 0xBF, 0x5F, 0x2E, 0xA9, 0xCF, 0x11, 0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
					guchar Content_Description_Object_GUID[] =
						{0x33, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
					guchar Extended_Content_Description_Object_GUID[] =
						{0x40, 0xA4, 0xD0, 0xD2, 0x07, 0xE3, 0xD2, 0x11, 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50};

					memcpy(asf_object_guid, &headerobject[offset], 16);
 				  //printf("Object GUID: %02X-%02X-%02X-%02X...\n", asf_object_guid[0], asf_object_guid[1], asf_object_guid[2], asf_object_guid[3]);

					if (headerobject[offset+20] != 0x00 ||
							headerobject[offset+21] != 0x00 ||
							headerobject[offset+22] != 0x00 ||
							headerobject[offset+23] != 0x00)
					{
						// We don't handle tags which require this magnitude of memory to read in!
						break;
					}
					asf_object_size = (headerobject[offset+19] << 24) + (headerobject[offset+18] << 16) + 
						(headerobject[offset+17] << 8) + headerobject[offset+16];
					//printf("Found object of size %lu\n", asf_object_size);
					if (guid_compare(asf_object_guid, File_Properties_Object_GUID))
					{
						//printf("Found File Properties Object\n");
						parse_file_properties(&headerobject[offset], asf_object_size, meta);
					}
					else if (guid_compare(asf_object_guid, Stream_Properties_Object_GUID))
					{
						//printf("Found Stream Properties Object\n");
					}
					else if (guid_compare(asf_object_guid, Header_Extension_Object_GUID))
					{
						//printf("Found Header Extension Object\n");
						parse_header_extension(&headerobject[offset], asf_object_size, meta);
					}
					else if (guid_compare(asf_object_guid, Content_Description_Object_GUID))
					{
						//printf("Found a Content Description Object\n");
						parse_content_description(&headerobject[offset], asf_object_size, meta);
					}
					else if (guid_compare(asf_object_guid, Extended_Content_Description_Object_GUID))
					{
						//printf("Found an Extended Content Description Object\n");
						parse_extended_content_description(&headerobject[offset], asf_object_size, meta);
					}
					offset += asf_object_size;
					current_object ++;
				}
			}
      free(headerobject);
    }
    /* Any embedded ID3V1 or ID3V2 tags will take
     * prescedence over any titles, artist names
     * etc gathered from analyzing the filename */
    //meta->artist = getArtist(tag);
    //meta->title = getTitle(tag);
    //meta->album = getAlbum(tag);
    //meta->genre = getGenre(tag);
    //meta->year = string_to_guint(getYear(tag));
    
    /* If there is a songlength tag it will take
     * precedence over any length calculated from
     * the bitrate and filesize */
    //songlen = getSonglen(tag);
    //if (songlen > 0) {
    //  meta->length = seconds_to_mmss(songlen);
    //} else {
    //  meta->length = NULL;
    //}
    //meta->trackno = string_to_guint(tracknum);
    //meta->filename = getOrigFilename(tag);
  }

  close(fd);
}
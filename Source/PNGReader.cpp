/***********************************************************************************

  Module :	PNGReader.cpp

  Description :

  Last Modified $Date: $

  $Revision: $

  Copyright (C) 2006 - PSPHacks.net Development Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  Contact information
  PSPHacks.net Development Team <webmaster@psphacks.net>
  71M - 71M@Orange.net

***********************************************************************************/

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include <libpng16/png.h>
#include <setjmp.h>
#include <stdlib.h>

#include "CFrameWork.h"
#include "CTextureManager.h"
#include "CFileSystem.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
CTexture * CTextureManager::ReadPNG( CFile * const p_file )
{
    const size_t sig_size = 8;
    png_byte signature[sig_size];

    if (p_file->Read(signature, sig_size) == false)
        return NULL;

    if (png_sig_cmp(signature, 0, sig_size) != 0)
        return NULL;

    png_structp png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL)
        return NULL;

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_init_io(png_ptr, *p_file);
    png_set_sig_bytes(png_ptr, sig_size);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    int compression_type, filter_type;

    png_get_IHDR(png_ptr, info_ptr, &width, &height,
        &bit_depth, &color_type, &interlace_type,
        &compression_type, &filter_type);

    bool has_alpha =
        (color_type & PNG_COLOR_MASK_ALPHA) != 0 ||
        png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0;

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    png_set_packing(png_ptr);
    png_set_expand(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    png_set_gray_to_rgb(png_ptr);

    if (!has_alpha)
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    png_bytep image_data = (png_bytep)malloc(rowbytes * height);
    png_bytep *rows = (png_bytep *)malloc(sizeof(png_bytep) * height);

    if (image_data == NULL || rows == NULL)
    {
        free(image_data);
        free(rows);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    for (png_uint_32 y = 0; y < height; ++y)
        rows[y] = image_data + y * rowbytes;

    png_read_image(png_ptr, rows);
    png_read_end(png_ptr, info_ptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    CTexture *p_texture = new CTexture();

    if (p_texture != NULL)
    {
        if (p_texture->Init(GU_PSM_8888, width, height) == false)
        {
            SAFE_DELETE(p_texture);
        }
        else
        {
            u8 *p_dest = p_texture->m_pBuffer;

            for (png_uint_32 y = 0; y < height; ++y)
            {
                const png_byte *row = rows[y];

                for (png_uint_32 x = 0; x < width; ++x)
                {
                    u8 r = row[x * 4 + 0];
                    u8 g = row[x * 4 + 1];
                    u8 b = row[x * 4 + 2];
                    u8 a = row[x * 4 + 3];

                    if (!has_alpha && r == 0 && g == 0 && b == 0)
                        a = 0;

                    p_dest[x * 4 + 0] = r;
                    p_dest[x * 4 + 1] = g;
                    p_dest[x * 4 + 2] = b;
                    p_dest[x * 4 + 3] = a;
                }

                p_dest += p_texture->m_nCanvasWidth * 4;
            }
        }
    }

    free(rows);
    free(image_data);

    return p_texture;
}

//*******************************  END OF FILE  ************************************

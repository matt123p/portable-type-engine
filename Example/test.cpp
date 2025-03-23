#include <fstream>
#include <cstdlib>
#include <iostream>
#include <vector>

extern "C"
{
#include "../include/pte.h"
	pte_base_font* get_Roboto128();
}

// Define our own RGB macro (format: 0x00RRGGBB)
#ifndef RGB
#define RGB(r, g, b) (((r) & 0xff) | (((g) & 0xff) << 8) | (((b) & 0xff) << 16))
#endif

// Global raw image buffer and dimensions
unsigned char* g_imageData = nullptr;
int g_width = 0;
int g_height = 0;

extern "C" void hw_blendPixel(int x, int y, int a, int col)
{
	if (x < 0 || x >= g_width || y < 0 || y >= g_height)
		return;
	int index = (y * g_width + x) * 3; // each pixel has 3 bytes (RGB)

	unsigned char p[3];
	p[0] = g_imageData[index];     // Red
	p[1] = g_imageData[index + 1]; // Green
	p[2] = g_imageData[index + 2]; // Blue

	unsigned int c[3];
	c[0] = col & 0xff;         // Blue component in our call order
	c[1] = (col >> 8) & 0xff;  // Green
	c[2] = (col >> 16) & 0xff; // Red

	int b = 256 - a;
	unsigned char newp[3];
	// Blend each channel
	newp[0] = ((p[0] * b) >> 8) + ((c[2] * a) >> 8); // Red
	newp[1] = ((p[1] * b) >> 8) + ((c[1] * a) >> 8); // Green
	newp[2] = ((p[2] * b) >> 8) + ((c[0] * a) >> 8); // Blue

	g_imageData[index] = newp[0];
	g_imageData[index + 1] = newp[1];
	g_imageData[index + 2] = newp[2];
}

int main(int argc, char* argv[])
{
	// Define image dimensions
	g_width = 640;
	g_height = 480;
	int size = g_width * g_height * 3; // 3 bytes per pixel

	// Allocate the raw image buffer and fill it with white (255, 255, 255)
	g_imageData = new unsigned char[size];
	for (int i = 0; i < size; i += 3)
	{
		g_imageData[i] = 255;     // Red
		g_imageData[i + 1] = 255; // Green
		g_imageData[i + 2] = 255; // Blue
	}

	int y = 0;

	// Draw first text line in a large font (green text)
	{
		pte_font f = pte_getFont(get_Roboto128(), 40);
		y = f.m_baseline;
		pte_drawText(&f, 5, y, 0, "Example text", -1, RGB(75, 255, 0));
		y += f.m_line_height;
	}

	// Draw a series of characters with increasing font sizes
	{
		int x = 0;
		char c[2] = "a";
		pte_font f;
		for (int s = 8; s < 32; s += 2)
		{
			f = pte_getFont(get_Roboto128(), s);
			x = pte_drawText(&f, x, y, 0, c, 1, RGB(0, 0, 0));
			++c[0];
		}
		y += f.m_line_height;
	}

	// Draw several lines of red text
	{
		pte_font f = pte_getFont(get_Roboto128(), 24);
		unsigned int colour = RGB(255, 0, 0);
		pte_drawText(&f, 5, y, 0, "I WANDERED lonely as a cloud", -1, colour);
		y += f.m_line_height;
		pte_drawText(&f, 5, y, 0, "That floats on high o'er vales and hills,", -1, colour);
		y += f.m_line_height;
		pte_drawText(&f, 5, y, 0, "When all at once I saw a crowd,", -1, colour);
		y += f.m_line_height;
		pte_drawText(&f, 5, y, 0, "A host, of golden daffodils;", -1, colour);
		y += f.m_line_height;
		pte_drawText(&f, 5, y, 0, "Beside the lake, beneath the trees,", -1, colour);
		y += f.m_line_height;
		pte_drawText(&f, 5, y, 0, "Fluttering and dancing in the breeze.", -1, colour);
	}

	// Draw a rotated text
	{
		pte_font f = pte_getFont(get_Roboto128(), 16);
		pte_drawText(&f, 540, 170, 0, "Rotated text", -1, RGB(0, 0, 0));
		pte_drawText(&f, 520, 170, 90, "Rotated text", -1, RGB(0, 0, 0));
		pte_drawText(&f, 510, 170, 180, "Rotated text", -1, RGB(0, 0, 0));
		pte_drawText(&f, 530, 150, 270, "Rotated text", -1, RGB(0, 0, 0));
	}

	// Save the final image to a BMP file
	{
		// Calculate row padding (each row must be a multiple of 4 bytes)
		int rowSize = g_width * 3;
		int paddingSize = (4 - (rowSize % 4)) % 4;
		int rowPadded = rowSize + paddingSize;
		uint32_t dataSize = rowPadded * g_height;

#pragma pack(push, 1)
		struct BMPFileHeader
		{
			uint16_t bfType;
			uint32_t bfSize;
			uint16_t bfReserved1;
			uint16_t bfReserved2;
			uint32_t bfOffBits;
		};

		struct BMPInfoHeader
		{
			uint32_t biSize;
			int32_t biWidth;
			int32_t biHeight;
			uint16_t biPlanes;
			uint16_t biBitCount;
			uint32_t biCompression;
			uint32_t biSizeImage;
			int32_t biXPelsPerMeter;
			int32_t biYPelsPerMeter;
			uint32_t biClrUsed;
			uint32_t biClrImportant;
		};
#pragma pack(pop)

		BMPFileHeader fileHeader;
		BMPInfoHeader infoHeader;

		fileHeader.bfType = 0x4D42; // 'BM'
		fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
		fileHeader.bfSize = fileHeader.bfOffBits + dataSize;
		fileHeader.bfReserved1 = 0;
		fileHeader.bfReserved2 = 0;

		infoHeader.biSize = sizeof(BMPInfoHeader);
		infoHeader.biWidth = g_width;
		// BMP images are stored bottom-up, so the height is positive.
		infoHeader.biHeight = g_height;
		infoHeader.biPlanes = 1;
		infoHeader.biBitCount = 24;
		infoHeader.biCompression = 0; // BI_RGB, no compression
		infoHeader.biSizeImage = dataSize;
		infoHeader.biXPelsPerMeter = 0;
		infoHeader.biYPelsPerMeter = 0;
		infoHeader.biClrUsed = 0;
		infoHeader.biClrImportant = 0;

		std::ofstream ofs("output.bmp", std::ios::binary);
		if (ofs)
		{
			// Write the BMP headers
			ofs.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
			ofs.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

			// Write pixel data. BMP requires rows to be written in bottom-up order.
			// Prepare a row buffer with padding.
			std::vector<unsigned char> rowData(rowPadded, 0);
			for (int i = g_height - 1; i >= 0; i--)
			{
				const unsigned char* rowPtr = g_imageData + (i * g_width * 3);
				// Copy row data into our temporary buffer.
				memcpy(rowData.data(), rowPtr, g_width * 3);
				// Write the full padded row.
				ofs.write(reinterpret_cast<const char*>(rowData.data()), rowPadded);
			}

			ofs.close();
			std::cout << "Image saved to output.bmp" << std::endl;
		}
		else
		{
			std::cout << "Failed to save BMP image" << std::endl;
			delete[] g_imageData;
			return -1;
		}
	}

	return 0;
}
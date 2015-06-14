#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

typedef struct fileHeader {
	uint16_t format;			// format of file; BM for bitmap
	uint16_t pad;				// padding
	uint32_t size;				// size of file, including header
	uint16_t reserved1;			// reserved; should set to 0 on file creation
	uint16_t reserved2;			// reserved; should set to 0 on file creation
	uint32_t pixelArrayLoc;			// offset of pixel array from beginning of file in bytes

} fileHeader;

typedef struct infoHeader {
	uint32_t size;				// size of headers
	int32_t width;				// width of image
	int32_t height;				// height of image
	uint16_t planes;			// number of planes
	uint16_t depth;				// color depth (1, 4, 8, 24)
	uint32_t compression;			// compression type (0:BI_RGB, 1:BI_RLE8, 2:BI_RLE4)
	uint32_t imgSize;			// size of pixel array in bytes
	uint32_t xRes;				// horizontal resolution
	uint32_t yRes;				// vertical resolution
	uint32_t colorTableSize;		// number of elements in color table
	uint32_t colorImportant;		// number of colors in the color table considered important for rendering
} infoHeader;

typedef struct chunk {
	uint32_t length;			//chunck length
	uint32_t chunkType;			//4 - byte chunk type code
	void *data;				//pointer to dynamically allocated memory space(?)
	uint32_t CRC;				//Cyclic Redundancy Check, always present
} chunk;

typedef struct ihdr {
	uint32_t width				//image width dimensions in pixels. 0 is invalid
	uint32_t height;			//image height dimensions in pixels. ~~~~~
	uint8_t bitDepth;			//num of bits p sample/per palette index
	uint8_t colorType;			//single-byte int that describes interpretation of the image data
	uint8_t compressionMethod;		//will always be 0 for png stnd
	uint8_t filterMethod;			//will always be 0 for png stnd
	uint8_t interlaceMethod;		//will always be 0 for png stnd
} ihdr

typedef struct plte {
	uint8_t red;				//(0 = black, 255 = red)
	uint8_t green;				//(0 = black, 255 = green)
	uint8_t blue;				//(0 = black, 255 = blue)
} plte;


int ReadBMP(FILE *begin, fileHeader *fh, infoHeader *ih, void *pixelArray);
// PRE:  begin points to the address of the first byte of a valid BMP image file
//       fh points to a chunk of memory that is the size of the struct fileHeader
//       ih points to a chunk of memory that is the size of the struct infoHeader
//       pixelArray points to null
// POST: fh points to a fileHeader containing the file header from the image file pointed to by begin
//       ih points to an infoHeader containing the info header from the image file pointed to by begin
//       begin points to the end of the image file's info header
//       pixelArray points to an array of color data that is in order of image rows from bottom to top,
//       with padding to justify word boundaries on the end of each row

int CreateIHDRChunk(infoHeader ih, chunk *target);
// PRE:  ih has valid data for a BMP info header, target points to null
// POST: target points to a chunk with code IHDR, with its data field pointing an ihdr struct
//       containing the relevant data from ih in line with the PNG spec, its length field 
//       containing the length of the data field in bytes, and its CRC field containing a valid CRC.

int CreatePLTEChunk(infoHeader ih, void *colorTable, chunk *target);
// PRE:  ih has valid data for a BMP info header, colorTable points to a BMP color palette,
//       target points to null.
// POST: target points to a chunk with code PLTE and length equal to the size of the data pointed to by
//       colorTable, target's data field points to a PNG color palette converted from the data from colorTable,
//       and its CRC field contains a valid CRC.

int CreateIDATChunk(infoHeader ih, void *pixelArray, chunk *target);
// PRE:  ih has valid data for a BMP info header, pixelArray points to BMP image data,
//       with scanlines aligned to byte boundaries and sorted bottom to top. target points to null.
// POST: target points to a chunk with code PLTE and length equal to the size of the data pointed to by
//       pixelArray, its data field points to PNG image data converted from the data pointed to by pixelArray,
//       with scanlines alinged to byte boundaries and sorted top to bottom

int CreateIENDChunk(chunk *target);
// POST: target points to a chunk with code IEND and length equal to 0

int BMPtoPNG(fileHeader fh, infoHeader ih, void *colorTable, void *pixelArray, FILE *target);
// PRE:  fh, ih, colorTable, and pixelArray are/point to the file header, info header, color table,
//       and pixel array of a single BMP image, respectively. target points to the beginning of an
//       open and empty file to store the PNG output of the converted BMP data.
// POST: target points to the end of a file containing the full PNG data converted from the BMP data
//       in the input

int CreateCRC(chunk *target);
// PRE:  target points to a chunk for which a CRC is needed
// POST: target's CRC field has a valid CRC that corresponds to its data

int main() {
	FILE *input;
	FILE *output;
	fileHeader fh;
	infoHeader ih;
	void *pixelArray = NULL;

	input = fopen("fireworks w crowd.bmp", "r");

	memset(&fh, 0x0, sizeof(fileHeader));
	memset(&ih, 0x0, sizeof(infoHeader));

	ReadBMP(input, &fh, &ih, pixelArray);

	fclose(input);

	/*printf("File Header\n");
	printf("format : %u\n", fh.format);
	printf("file size : %u\n", fh.size);
	printf("reserved1 : %u\n", fh.reserved1);
	printf("reserved2 : %u\n", fh.reserved2);
	printf("pixel array offset : %u\n", fh.pixelArrayLoc);

	printf("Info Header\n");
	printf("size : %u\n", ih.size);
	printf("width : %u\n", ih.width);
	printf("height : %u\n", ih.height);
	printf("planes : %u\n", ih.planes);
	printf("depth : %u\n", ih.depth);
	printf("compression : %u\n", ih.compression);
	printf("imgSize : %u\n", ih.imgSize);
	printf("xRes : %u\n", ih.xRes);
	printf("yRes : %u\n", ih.yRes);
	printf("colorTableSize : %u\n", ih.colorTableSize);
	printf("colorImportant : %u\n", ih.colorImportant);*/

	output = fopen("fireworks w crowd.png", "w");
	fclose(output);

	return 0;
}

int ReadBMP(FILE *begin, fileHeader *fh, infoHeader *ih, void *pixelArray)
{
	int i;
	int j;
	int overflow;
	int rowPadding;
	int rowSize;

	for (i = 0; i < sizeof(fileHeader); i++)
	{
		if (i != 2 && i != 3)
			fscanf(begin, "%c", (char *)(fh) + i);
	}
	for (i = 0; i < sizeof(infoHeader); i++)
		fscanf(begin, "%c", (char *)(ih) + i);

	rowDataSize = ((ih->width * ih->depth) / 8);

	overflow = rowDataSize % 4;

	if (overflow != 0)
		rowPadding = 4 - overflow;

	rowSize = rowDataSize + rowPadding;

	pixelArray = malloc(rowSize * ih->height);

	for (i = 0; i < ih->height * rowSize; i++)
		fscanf(begin, "%c", (char *)(pixelArray + i);
	}

	return 0;
}

int CreateCRC(chunk *target)
{
	uint32_t crc = crc32(0L, Z_NULL, 0);

	crc = crc32(crc, target->data, target->length);

	target->CRC = crc;

	return 0;
}

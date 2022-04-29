// kantfilter.cpp, Thomas Lundqvist 2019-2022, Use freely!
//
// OBS: ej färdigt, skriv färdigt denna!
//
// Använder filesystem från C++17 standardarden
// Eventuellt behöver man ställa in detta i projektinställningarna
// (general properties, c++ language standard)

#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>

using namespace std;

//
// Anropa denna för att enkelt kunna få åäö rätt utskriva (Windows)
//
void setWesternEuropeanCodePage() {
	SetConsoleOutputCP(1252);
	SetConsoleCP(1252);
}

//
// Hjälpfunktion för hexdump. Bra för felsökning.
// Skriv ut en hexdump av alla bytes mellan [start, start+maxbytes-1]
//
void hexdump(const unsigned char* start, int maxbytes) {
	for (int r = 0; r < maxbytes / 16 + 1; r++) {
		cout << setw(4) << hex << setfill('0');
		cout << r * 16 << ": ";
		for (int i = 0; i < 16; i++) {
			int idx = i + r * 16;
			if (idx < maxbytes) {
				cout << setw(2) << hex << setfill('0');
				cout << (int)start[idx] << ' ';
			}
			else {
				cout << "   ";
			}
			if (i == 7)
				cout << ' ';
		}
		cout << "  ";
		for (int i = 0; i < 16; i++) {
			int idx = i + r * 16;
			if (idx < maxbytes) {
				cout << (start[idx] < 32 ? '.' : (char)start[idx]);
			}
		}
		cout << dec << endl;
	}
}

struct bild {
	int offset, width, height;
};

// Förslag på funktion: parseBMP()
// Kan vara proffsigt med någon slags funktion som plockar ut rätt saker från rådatan.
// BMP-formatet är rätt bra beskrivet på:
//   https://en.wikipedia.org/wiki/BMP_file_format
// Filerna använder BITMAPV5HEADER och alla pixlar lagras som 3 bytes i ordningen BGR.
// Varje färg Röd, Grön eller Blå är alltså 8-bitar (0-255) som talar om styrkan på färgen.
// 
void parseBMP(const unsigned char* rawimage, bild* greyscale) {
	// Plocka ut lämpliga fält ur orginalbilddatan.
	// Exempel, plocka ut ett 4-bytes heltal från position 14:
	//int tal = *(int *)(rawimage + 14);

	greyscale->offset = *(int*)(rawimage + 10);
	greyscale->width = *(int*)(rawimage + 18);
	greyscale->height = *(int*)(rawimage + 22);

	// (obs: osäker kod egentligen, vi litar då på att int är 4 bytes och 
	//  att maskinen är little endian precis som BMP-formatet använder)
	// Returnera allt intressant i en struct eller i globala variabler.
}

//
// Gör om alla färgpixlar till gråskala. Spara över i samma buffert eller till en ny bildbuffert.
//
void convert_greyscale(int width, int height, unsigned char* pixels) {
	for (int i = 0; i < width * height; i++) {
		int R = *(pixels + 2);
		int G = *(pixels + 1);
		int B = *(pixels);
		int Y = 0.2126 * R + 0.7152 * G + 0.0722 * B;/*
		cout << "\nPixel: " << *pixels << "\n"; //PRINTAR UT PIXLAR
		cout << "\nPixel + 1: " << *pixels + 1 << "\n"; //PRINTAR UT PIXLAR
		cout << "\nPixel + 2: " << *pixels + 2 << "\n"; //PRINTAR UT PIXLAR*/
		*pixels = Y;
		*(pixels + 1) = Y;
		*(pixels + 2) = Y;
		pixels += 3;
	}
	// eller kanske:
	// void convert_greyscale(int width, int height, const unsigned char *srcpixels, unsigned char *dstpixels) {
		// Finns olika sätt, välj ett:
		// 1. sätt färgerna R och B till samma som G (grönt är viktigast för oss människor)
		// 2. eller använd en proffsigare formel (https://en.wikipedia.org/wiki/Grayscale#Converting_color_to_grayscale)
		//    Y = 0.2126 * R + 0.7152 * G + 0.0722 * B
}

//
// Hälpfunktion till sobelfiltret.
// Returnera pixelvärdet på rad "row" (y) och kolumn "col" (x).
// Antag att greypixels ska peka på första byten i första pixeln
//
unsigned char get_pixel(const unsigned char* greypixels, int width, int row, int col) {
	int pixel = width * row + col;
	return *(greypixels + pixel * 3);
}
//
// Hälpfunktion till sobelfiltret.
// Skriv nytt pixelvärdet på rad "row" (y) och kolumn "col" (x).
//
void set_pixel(unsigned char* dstpixels, int width, int row, int col, unsigned char newval) {
	int pix = width * row + col;
	*(dstpixels + pix * 3) = newval;
	*(dstpixels + pix * 3 + 1) = newval;
	*(dstpixels + pix * 3 + 2) = newval;

	// Bör skriva alla färgkomponenter R, G, B till samma "newval"
	// (om dstpixels är en RGB-bild)
}


//
// Omvandla src-bilden till en dst-bild med kanter markerade med ljusare gråskala.
// Använder ett Sobelfilter: https://en.wikipedia.org/wiki/Sobel_operator
//
void filter_sobel(int start, int stop, int width, int height, const unsigned char* greypixels, unsigned char* dstpixels) {
	int sobelX[3][3] = {
		{ -1, 0, 1 },
		{ -2, 0, 2 },
		{ -1, 0, 1 } };
	int sobelY[3][3] = {
		{ -1, -2, -1 },
		{ 0,  0,  0 },
		{ 1,  2,  1 } };

	// Compute a new pixel for every row and column.
	// Skip edges since the convolution reads neighboring pixels.
	for (int r = 1; r < height - 1; r++)
		for (int c = 1; c < width - 1; c++) {
			// For each pixel, convolve kernel with neighbors
			// Kernels should maybe be flipped (in x) but this doesn't really matter for edge detection

			// Horizontal convolution
			int sumX = 0;
			for (int sx = 0; sx < 3; sx++)
				for (int sy = 0; sy < 3; sy++) {
					sumX += sobelX[sy][sx] * get_pixel(greypixels, width, r + sy - 1, c + sx - 1);
					// Alternativ till get_pixel() är att ha någon fiffig array direkt. T ex:
					//   sumX += sobelX[sy][sx] * pixel[r + sy - 1][c + sx - 1].G;
				}
			// Vertical convolution
			int sumY = 0;
			for (int sx = 0; sx < 3; sx++)
				for (int sy = 0; sy < 3; sy++) {
					sumY += sobelY[sy][sx] * get_pixel(greypixels, width, r + sy - 1, c + sx - 1);
					// sumY += sobelY[sy][sx] * pixel[r + sy - 1][c + sx - 1].G;
				}
			// Compute combined sum and limit to 255
			int sum = (int)sqrt((double)sumX * sumX + (double)sumY * sumY);
			if (sum > 255)
				sum = 255;
			set_pixel(dstpixels, width, r, c, sum);
			// Alternativ till set_pixel är att ha någon fiffig array. T ex:
			//   dest_pixel[r][c].R = (unsigned char)sum;
			//   dest_pixel[r][c].G = (unsigned char)sum;
			//   dest_pixel[r][c].B = (unsigned char)sum;
		}
}


int main(int argc, char** argv)
{
	setWesternEuropeanCodePage();
	bild sobel;

	if (argc < 3) {
		cerr << "Ange infil och utfil som två argument!" << endl;
		return 1;
	}
	cout << "Läser från " << argv[1] << endl;
	cout << "Skriver till: " << argv[2] << endl;

	// Exempel på filläsning och skrivning, man kan ha bättre felhantering!
	unsigned infil_size = (unsigned)filesystem::file_size(argv[1]);
	cout << "Storlek: " << infil_size << " bytes" << endl << endl;
	unsigned char* rawimage = new unsigned char[infil_size];

	// Läs alla bytes. Notera "binary" här är viktigt.
	ifstream infil(argv[1], fstream::binary);
	infil.read((char*)rawimage, infil_size);
	infil.close();

	cout << endl << "Hexdump av bildfilen (för debuggning):" << endl;
	hexdump(rawimage, 200);
	cout << endl;

	// Gör en exakt kopia till destimage så följer all data i bildhuvudet
	// med automatiskt. Själva pixlarna ska skrivas över av sobelfiltret
	// senare.
	unsigned char* destimage = new unsigned char[infil_size];
	memcpy(destimage, rawimage, infil_size);

	parseBMP(rawimage, &sobel);

	cout << "Konverterar till gråskala och kör sobelfiltret..." << endl;
	LARGE_INTEGER startcount, stopcount, frequency;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&startcount);

	convert_greyscale(sobel.width, sobel.height, rawimage + sobel.offset);

	int rader = sobel.height - 2;
	int trådar = 8;
	int rpt = (rader / trådar);

	thread t1(filter_sobel, 1, rpt + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t2(filter_sobel, rpt + 1, rpt * 2 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t3(filter_sobel, rpt * 2 + 1, rpt * 3 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t4(filter_sobel, rpt * 3 + 1, rpt * 4 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t5(filter_sobel, rpt * 4 + 1, rpt * 5 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t6(filter_sobel, rpt * 5 + 1, rpt * 6 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t7(filter_sobel, rpt * 6 + 1, rpt * 7 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t8(filter_sobel, rpt * 7 + 1, sobel.height - 1, sobel.width, sobel.height, rawimage, destimage);
	/*thread t9(filter_sobel, rpt * 8 + 1, rpt * 9 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t10(filter_sobel, rpt * 9 + 1, rpt * 10 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t11(filter_sobel, rpt * 10 + 1, rpt * 11 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t12(filter_sobel, rpt * 11 + 1, rpt * 12 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t13(filter_sobel, rpt * 12 + 1, rpt * 13 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t14(filter_sobel, rpt * 13 + 1, rpt * 14 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t15(filter_sobel, rpt * 14 + 1, rpt * 15 + 1, sobel.width, sobel.height, rawimage, destimage);
	thread t16(filter_sobel, rpt*15+1, sobel.height - 1, sobel.width, sobel.height, rawimage, destimage);*/
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	t8.join();
	/*t9.join();
	t10.join();*/
	//t11.join();
	//t12.join();
	//t13.join();
	//t14.join();
	//t15.join();
	//t16.join();

	QueryPerformanceCounter(&stopcount);
	cout << "Klar!" << endl;
	cout << "Start: " << startcount.QuadPart << "  (freq = " << frequency.QuadPart << ")" << endl;
	cout << "Stopp: " << stopcount.QuadPart << endl;
	cout << "Diff: " << stopcount.QuadPart - startcount.QuadPart << endl;
	cout << "Tid: " << (double)(stopcount.QuadPart - startcount.QuadPart) / frequency.QuadPart * 1000 << " ms" << endl;

	// Skriv destimage som nu är uppdaterad med nya pixelvärden
	ofstream utfil(argv[2], fstream::binary);
	utfil.write((char*)destimage, infil_size);
	utfil.close();

	delete[] rawimage;
	delete[] destimage;
	return 0;
}

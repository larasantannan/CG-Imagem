//This program loads 3-channel BMP
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define TAM_JANELA 9;

using namespace std;

// bitmap header structure, has to be packed to avoid compiler padding
#pragma pack(1)
typedef struct BITMAPFILEHEADER {
  char magic[2];       // "BM" 0x424d - char[2] to avoid endian problems
  uint32_t filesize;   // size of the bitmap file (data + headers)
  uint16_t reserved1;  // must be 0
  uint16_t reserved2;  // must be 0
  uint32_t dataoffset; // when does the data start
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER {
  uint32_t headersize;      // size of this header
  int32_t width;            // width of the bmp
  int32_t height;           // height of the bmp
  uint16_t colorplanes;     // must be 1
  uint16_t bitdepth;        // bits per pixel
  uint32_t compression;     // 0 = uncompressed
  uint32_t imagesize;       // can be 0 if bmp is uncompressed
  int32_t hresolution;      // print resolution
  int32_t vresolution;      // print resolution
  uint32_t palettecolors;   // can be 0 if uncompressed
  uint32_t importantcolors; // number of important colors, 0 = all are important
} BITMAPINFOHEADER;

typedef struct BITMAPFULLHEADER {
  BITMAPFILEHEADER fileinfo;
  BITMAPINFOHEADER bmpinfo;
} BITMAPFULLHEADER;
#pragma pack(0)

// BMP HEADER
BITMAPFULLHEADER header;

// image data
unsigned char *data;        // loaded image
vector< vector<double> > imagem;

int fp_pixel = 3;
int fp_cor = 8;
vector<double> janela[TAM_JANELA][TAM_JANELA];

double distancia(int px, int py, int qx, int qy) {
  double d;
  d = pow(px-qx, 2) + pow(py-qy, 2);
  d = sqrt(d);

  return d;
}

double funcaoGaussiana(double x, int fp) {
  double resultado;
  double fp_quadrado = pow(fp, 2);
  double x_quadrado = pow(x, 2);

  resultado = 1/(2*M_PI*fp_quadrado);
  double exponente = (-1)*(x_quadrado/(2*fp_quadrado));
  resultado *= pow(M_E, exponente);

  return resultado;
}

double fatorNormalizacao(int px, int py, double intensidade_p) {
  double wp = 0.0;
  for(int i=0; i<TAM_JANELA; i++) {
    for(int j=0; j<TAM_JANELA; j++) {
      vector<double> pixel = janela[i][j];

      if(pixel[0] != -1){
        double d = distancia(px, py, pixel[0], pixel[1]);
        wp_2 += funcaoGaussiana(d, fp_pixel) * funcaoGaussiana(intensidade_p - pixel[2], fp_cor);
      }
    }
  }

  return wp;
}

double fatorNormalizacaoComIq(int px, int py, double intensidade_p) {
  double wp = 0.0;
  for(int i=0; i<TAM_JANELA; i++) {
    for(int j=0; j<TAM_JANELA; j++) {
      vector<double> pixel = janela[i][j];

      if(pixel[0] != -1) {
        double d = distancia(px, py, pixel[0], pixel[1]);
        wp += funcaoGaussiana(d, fp_pixel) * funcaoGaussiana(intensidade_p - pixel[2], fp_cor) * pixel[2];
      }
    }
  }

  return wp;
}

void constroiJanela(int x, int y) {
  for(int i=0; i<TAM_JANELA; i++){
    for(int j=0; j<TAM_JANELA; j++){
      vector<double> pixel;
      pixel.append(-1);
      pixel.append(-1);
      pixel.append(-1);
      janela[i][j] = pixel;
    }
  }

  vector<double> pixel;
  pixel.append(x);
  pixel.append(y);
  pixel.append(imagem[x][y]);
  janela[x][y] = pixel;

  pixel.clear();
  if(x-1 >= 0) {
    pixel.append(x-1);
    pixel.append(y);
    pixel.append(imagem[x-1][y]);
    janela[x-1][y] = pixel;

    if(y-1 >= 0) {
      pixel.clear();
      pixel.append(x-1);
      pixel.append(y-1);
      pixel.append(imagem[x-1][y-1]);
      janela[x-1][y-1] = pixel;
    }

    if(y+1 >= imagem[0].size()-1) {
      pixel.clear();
      pixel.append(x-1);
      pixel.append(y+1);
      pixel.append(imagem[x-1][y+1]);
      janela[x-1][y+1] = pixel;
    }
  }

  if(x+1 <= imagem.size()-1) {
    pixel.append(x+1);
    pixel.append(y);
    pixel.append(imagem[x+1][y]);
    janela[x+1][y] = pixel;

    if(y-1 >= 0) {
      pixel.clear();
      pixel.append(x+1);
      pixel.append(y-1);
      pixel.append(imagem[x+1][y-1]);
      janela[x+1][y-1] = pixel;
    }

    if(y+1 >= imagem[0].size()-1) {
      pixel.clear();
      pixel.append(x+1);
      pixel.append(y+1);
      pixel.append(imagem[x+1][y+1]);
      janela[x+1][y+1] = pixel;
    }
  }

  if(y-1 >= 0) {
    pixel.clear();
    pixel.append(x);
    pixel.append(y-1);
    pixel.append(imagem[x][y-1]);
    janela[x][y-1] = pixel;
  }

  if(y+1 >= imagem[0].size()-1) {
    pixel.clear();
    pixel.append(x);
    pixel.append(y+1);
    pixel.append(imagem[x][y+1]);
    janela[x][y+1] = pixel;
  }
}

double BF() {
  for(int i=0; i<imagem.size(); i++){
    for(int j=0; j<imagem[0].size(); j++){
      constroiJanela(i, j);

      double wp = fatorNormalizacao(i, j, imagem[i][j]);
      double wp_2 = fatorNormalizacaoComIq(i, j, imagem[i][j]);
      double bf = (1/wp)*wp_2;

      return bf;
    }
  }
}


// dynamic programming to improve performance

int loadBMP(const char *imagepath) {
  // Open the file
  FILE *file = fopen(imagepath, "rb");
  if (!file) {
    printf("Image could not be opened\n");
    return 0;
  }

  // Read header
  if (fread(&header, 1, sizeof(BITMAPFULLHEADER), file) !=
      54) { // If not 54 bytes read : problem
    printf("Not a correct BMP file - Wrong header size\n");
    return 1;
  }

  if (header.fileinfo.magic[0] != 'B' || header.fileinfo.magic[1] != 'M') {
    printf("Not a correct BMP file - Wrong magic number\n");
    return 1;
  }

  // Read ints from the header
  unsigned long dataPos = header.fileinfo.dataoffset;
  unsigned long imageSize = header.bmpinfo.imagesize;
  long width = header.bmpinfo.width;
  long height = header.bmpinfo.height;

  // Some BMP files are misformatted, guess missing information
  if (imageSize == 0) {
    imageSize = width * height * 3;
  }
  if (dataPos == 0) {
    dataPos = 54; // The BMP header is usually done that way
  }

  // Allocate buffer
  data = (unsigned char *)malloc(imageSize);

  // Read the actual data from the file into the buffer
  fseek(file, dataPos, SEEK_SET);
  if (!fread(data, 1, imageSize, file)) {
    printf("Couldn't read BMP Data. Giving up.\n");
    return 1;
  }

  // copying components BGR to RGB
  unsigned char B,R;
  for(int i = 0; i < width * height ; i++){
   int index = i*3;
   B = data[index];
   R = data[index+2];
   data[index] = R;
   data[index+2] = B;
  }

  fclose(file);
  return 0;
}


void init(void) { 
  glClearColor(0.0, 0.0, 0.0, 0.0); 
}

void display(void) {
  long width = header.bmpinfo.width;
  long height = header.bmpinfo.height;

  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0, 0);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
  glRasterPos2i(width, 0);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
  glFlush();
}

void reshape(int w, int h) {
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, 0, h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
  case 27:
    exit(0);
  }
}

int main(int argc, char **argv) {
  loadBMP(argv[1]);
  long width = header.bmpinfo.width;
  long height = header.bmpinfo.height;
  //double imagem[width][height];
  imagem.resize(width);
  for (int i = 0; i<width*height*3; i+=height*3){
    for (int j = 0; j<3*height; j+=3){
      //imagem[i/(height*3)][j/3] = data[i+j]/255.0;
      imagem[i/(height*3)].push_back(data[i+j]/255.0);
    }
  }
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(width * 2, height);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  init();
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;
}
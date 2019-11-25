//This program loads 3-channel BMP
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

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
vector< vector<double> > imagem2;

int fp_pixel = 3;
int fp_cor = 8;
vector<double> janela[9][9];

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
  for(int i=0; i<9; i++) {
    for(int j=0; j<9; j++) {
      vector<double> pixel = janela[i][j];

      if(pixel[0] != -1){
        double d = distancia(px, py, pixel[0], pixel[1]);
        wp += funcaoGaussiana(d, fp_pixel) * funcaoGaussiana(intensidade_p - pixel[2], fp_cor);
      }
    }
  }

  return wp;
}

double fatorNormalizacaoComIq(int px, int py, double intensidade_p) {
  double wp = 0.0;
  for(int i=0; i<9; i++) {
    for(int j=0; j<9; j++) {
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
  for(int i=0; i<9; i++){
    for(int j=0; j<9; j++){
      vector<double> pixel;
      pixel.push_back(-1);
      pixel.push_back(-1);
      pixel.push_back(-1);
      janela[i][j] = pixel;
    }
  }

  int count_x = 0, count_y = 0;
  for(int i = x - 4; i <= x + 4; i++){
    for(int j = y - 4; j <= y + 4; j++){
      vector<double> pixel;
      pixel.push_back(i);
      pixel.push_back(j);
      pixel.push_back(imagem2[i][j]);
      janela[count_x][count_y] = pixel;
      count_y++;
    }
    count_x++;
  }

}

double BF() {
  for(int i=4; i<imagem2.size() - 4; i++){
    for(int j=4; j<imagem2[0].size() - 4; j++){
      constroiJanela(i, j);
      double wp = fatorNormalizacao(i, j, imagem2[i][j]);
      double wp_2 = fatorNormalizacaoComIq(i, j, imagem2[i][j]);
      double bf = (1/wp)*wp_2;

      imagem[i - 4][j - 4] = bf;
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

void equalizacao(){
    double niveis[256];
    double prob[256];
    int n_k[256];
    double s[256];
    for (int i = 0; i<256; i++){
        niveis[i] = i/255.0;
        n_k[i] = 0;
        s[i] = 0;
    }
    for (int i = 0; i<imagem.size(); i++){
        for (int j = 0; j<imagem[i].size(); j++){
            n_k[(int)imagem[i][j]*255]++;
        }
    }
    for (int i = 0; i<256; i++){
        prob[i] = n_k[i]/255.0;
        for (int j = 0; j<=i; j++){
            s[i] += prob[j];
        }
        double mais_prox = 0;
        double menor_dif = 2;
        for (int j = 0; j<256; j++){
            double nivel = j/255.0;
            double dif_atual = fabs(nivel-s[i]);
            if (dif_atual < menor_dif){
                menor_dif = dif_atual;
                mais_prox = nivel;
            }
        }
        for (int l = 0; l<imagem.size(); l++){
            for (int c = 0; c<imagem[i].size(); c++){
                if (imagem[l][c] == i/255.0){
                    imagem[l][c] = mais_prox;
                }
            }
        }
    }
    // salvar e mostrar imagem
    /*for (int i = 0; i<imagem.size(); i++){
        for (int j = 0; j<imagem[i].size(); j++){
            cout << imagem[i][j]*255 << " ";
        }
        cout << endl;
    }*/
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
  //BF();
  equalizacao();
  imagem2.clear();
  unsigned char data2[height*width*3];     
  for (int i = 0; i<width; i++){
    for (int j = 0; j<height; j++){
      data2[i*height*3 + j*3] = imagem[i][j]*255;
      data2[i*height*3 + j*3 + 1] = imagem[i][j]*255;
      data2[i*height*3 + j*3 + 2] = imagem[i][j]*255;
    }
  }

  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data2);
  glRasterPos2i(width*2, 0);
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
  long width2 = header.bmpinfo.width + 8;
  long height2 = header.bmpinfo.height + 8;
  //double imagem[width][height];

  imagem2.resize(width2);
  for (int i = 0; i<width2; i++){
    for (int j = 0; j<height2; j++){
      imagem2[i].push_back(-1);
    }
  }

  imagem.resize(width);
  for (int i = 0; i<width*height*3; i+=height*3){
    for (int j = 0; j<3*height; j+=3){
      imagem2[i/(height*3) + 4][j/3 + 4] = data[i+j]/255.0;
      imagem[i/(height*3)].push_back(data[i+j]/255.0);
    }
  }
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(width * 3, height);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  init();
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;
}
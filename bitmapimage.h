#pragma once

#include <stdio.h>

class BitmapImage
{
public:
    BitmapImage(const char* fileName) {
        // Данные, прочитанные из заголовка BMP-файла
        unsigned char header[54]; // Каждый BMP-файл начинается с заголовка, длиной в 54 байта
        unsigned int dataPos;     // Смещение данных в файле (позиция данных)

        FILE * file = fopen(fileName,"rb");
        if (!file) {
          printf("Изображение не может быть открытоn");
          return;
        }

        if ( fread(header, 1, 54, file) != 54 ) { // Если мы прочитали меньше 54 байт, значит возникла проблема
            printf("Некорректный BMP-файлn");
            return;
        }

        if ( header[0]!='B' || header[1]!='M' ){
            printf("Некорректный BMP-файлn");
            return;
        }

        // Читаем необходимые данные
        dataPos    = *(int*)&(header[0x0A]); // Смещение данных изображения в файле
        imageSize  = *(int*)&(header[0x22]); // Размер изображения в байтах
        width      = *(int*)&(header[0x12]); // Ширина
        height     = *(int*)&(header[0x16]); // Высота

        // Некоторые BMP-файлы имеют нулевые поля imageSize и dataPos, поэтому исправим их
        if (imageSize==0)    imageSize=width*height*3; // Ширину * Высоту * 3, где 3 - 3 компоненты цвета (RGB)
        if (dataPos==0)      dataPos=54; // В таком случае, данные будут следовать сразу за заголовком

        // Создаем буфер
        data = new unsigned char [imageSize];

        // Считываем данные из файла в буфер
        fread(data,1,imageSize,file);

        // Закрываем файл, так как больше он нам не нужен
        fclose(file);
    }
    unsigned char* getData() {
        return data;
    }
    int getDataSize() {
        return imageSize;
    }
    int getWidth() {
        return width;
    }
    int getHeight() {
        return height;
    }
private:
    unsigned char* data;
    int imageSize;
    int width;
    int height;
};

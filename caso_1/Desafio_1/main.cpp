#include <fstream>
#include <iostream>
#include <QCoreApplication>
#include <QImage>
#include <filesystem>
using namespace std;
unsigned char* loadPixels(QString input, int &width, int &height);
bool exportImage(unsigned char* pixelData, int width,int height, QString archivoSalida);
unsigned int* loadSeedMasking(const char* nombreArchivo, int &seed, int &n_pixels);
unsigned char* loadPixels(QString input, int &width, int &height){
    // Cargar la imagen BMP desde el archivo especificado (usando Qt)
    QImage imagen(input);

    std::filesystem::path current = std::filesystem::current_path();
    std::cout << current.string() << "\n";

    // Verifica si la imagen fue cargada correctamente
    if (imagen.isNull()) {
        cout << "Error: No se pudo cargar la imagen BMP." << std::endl;
        return nullptr; // Retorna un puntero nulo si la carga falló
    }// Variables para almacenar la semilla y el número de píxeles leídos del archivo de enmascaramiento
    // Convierte la imagen al formato RGB888 (3 canales de 8 bits sin transparencia)
    imagen = imagen.convertToFormat(QImage::Format_RGB888);
    // Obtiene el ancho y el alto de la imagen cargada
    width = imagen.width();
    height = imagen.height();
    // Calcula el tamaño total de datos (3 bytes por píxel: R, G, B)
    int dataSize = width * height * 3;

    // Reserva memoria dinámica para almacenar los valores RGB de cada píxel
    unsigned char* pixelData = new unsigned char[dataSize];
    // Copia cada línea de píxeles de la imagen Qt a nuestro arreglo lineal
    for (int y = 0; y < height; ++y) {
        const uchar* srcLine = imagen.scanLine(y);              // Línea original de la imagen con posible padding
        unsigned char* dstLine = pixelData + y * width * 3;     // Línea destino en el arreglo lineal sin padding
        memcpy(dstLine, srcLine, width * 3);                    // Copia los píxeles RGB de esa línea (sin padding)
    }
    // Retorna el puntero al arreglo de datos de píxeles cargado en memoria
    return pixelData;
}
bool exportImage(unsigned char* pixelData, int width,int height, QString archivoSalida){
    // Crear una nueva imagen de salida con el mismo tamaño que la original
    // usando el formato RGB888 (3 bytes por píxel, sin canal alfa)
    QImage outputImage(width, height, QImage::Format_RGB888);

    // Copiar los datos de píxeles desde el buffer al objeto QImage
    for (int y = 0; y < height; ++y) {
        // outputImage.scanLine(y) devuelve un puntero a la línea y-ésima de píxeles en la imagen
        // pixelData + y * width * 3 apunta al inicio de la línea y-ésima en el buffer (sin padding)
        // width * 3 son los bytes a copiar (3 por píxel)
        memcpy(outputImage.scanLine(y), pixelData + y * width * 3, width * 3);
    }

    // Guardar la imagen en disco como archivo BMP
    if (!outputImage.save(archivoSalida, "BMP")) {
        // Si hubo un error al guardar, mostrar mensaje de error
        cout << "Error: No se pudo guardar la imagen BMP modificada.";
        return false; // Indica que la operación falló
    } else {
        // Si la imagen fue guardada correctamente, mostrar mensaje de éxito
        cout << "Imagen BMP modificada guardada como " << archivoSalida.toStdString() << endl;
        return true; // Indica éxito
    }

}
unsigned int* loadSeedMasking(const char* nombreArchivo, int &seed, int &n_pixels){
    // Abrir el archivo que contiene la semilla y los valores RGB
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        // Verificar si el archivo pudo abrirse correctamente
        cout << "No se pudo abrir el archivo." << endl;
        return nullptr;
    }

    // Leer la semilla desde la primera línea del archivo
    archivo >> seed;

    int r, g, b;

    // Contar cuántos grupos de valores RGB hay en el archivo
    // Se asume que cada línea después de la semilla tiene tres valores (r, g, b)
    while (archivo >> r >> g >> b) {
        n_pixels++;  // Contamos la cantidad de píxeles
    }

    // Cerrar el archivo para volver a abrirlo desde el inicio
    archivo.close();
    archivo.open(nombreArchivo);

    // Verificar que se pudo reabrir el archivo correctamente
    if (!archivo.is_open()) {
        cout << "Error al reabrir el archivo." << endl;
        return nullptr;
    }

    // Reservar memoria dinámica para guardar todos los valores RGB
    // Cada píxel tiene 3 componentes: R, G y B
    unsigned int* RGB = new unsigned int[n_pixels * 3];

    // Leer nuevamente la semilla desde el archivo (se descarta su valor porque ya se cargó antes)
    archivo >> seed;

    // Leer y almacenar los valores RGB uno por uno en el arreglo dinámico
    for (int i = 0; i < n_pixels * 3; i += 3) {
        archivo >> r >> g >> b;
        RGB[i] = r;
        RGB[i + 1] = g;
        RGB[i + 2] = b;
    }

    // Cerrar el archivo después de terminar la lectura
    archivo.close();

    // Mostrar información de control en consola
    cout << "Semilla: " << seed << endl;
    cout << "Cantidad de píxeles leídos: " << n_pixels << endl;

    // Retornar el puntero al arreglo con los datos RGB
    return RGB;
}

void aplicarXOR(unsigned char* img1, unsigned char* img2, unsigned char* resultado, int size) {
    for (int i = 0; i < size; i++) {
        resultado[i] = img1[i] ^ img2[i];
    }
}
unsigned char rotarDerecha(unsigned char byte, int bits) {
    return (byte >> bits) | (byte << (8 - bits));
}
void rotarImagen(unsigned char* entrada, unsigned char* salida, int size, int bits) {
    for (int i = 0; i < size; i++) {
        salida[i] = rotarDerecha(entrada[i], bits);
    }
}
bool verificarEnmascaramiento(unsigned char* generado, unsigned char* mascara, unsigned int* valoresTXT, int seed, int n_pixels) {
    for (int i = 0; i < n_pixels * 3; i++) {
        if ((int)(generado[i + seed] + mascara[i]) != valoresTXT[i]) {
            return false;
        }
    }
    return true;

}

// CASO 1
int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    int width_IO = 0, height_IO = 0;
    unsigned char* I_O = loadPixels("I_O.bmp", width_IO, height_IO);
    int size_IO = width_IO * height_IO * 3;
    int width_IM = 0, height_IM = 0;
    unsigned char* I_M = loadPixels("I_M.bmp", width_IM, height_IM);
    int size_IM = width_IM * height_IM * 3;
    int width_M = 0, height_M = 0;
    unsigned char* M   = loadPixels("M.bmp", width_M, height_M);

    unsigned char* P1 = new unsigned char[size_IO];
    aplicarXOR(I_O, I_M, P1, size_IO);
    int seed1 = 0, n1 = 0;
    unsigned int* M1_txt = loadSeedMasking("M1.txt", seed1, n1);
    if (!verificarEnmascaramiento(P1, M, M1_txt, seed1, n1)) {
        cout << "Error: M1.txt no coincide" << endl;
        return 0;
    }

    unsigned char* P2 = new unsigned char[size_IO];
    rotarImagen(P1, P2, size_IO, 3);
    int seed2 = 0, n2 = 0;
    unsigned int* M2_txt = loadSeedMasking("M2.txt", seed2, n2);
    if (!verificarEnmascaramiento(P2, M, M2_txt, seed2, n2)) {
        cout << "Error: M2.txt no coincide" << endl;
        return 0;
    }

    unsigned char* P3 = new unsigned char[size_IM];
    aplicarXOR(P2, I_M, P3, size_IM);

   //Reconstrucción Caso 1
    unsigned char* R1 = new unsigned char[size_IM];
    aplicarXOR(P3, I_M, R1, size_IM);
    unsigned char* R2 = new unsigned char[size_IM];
    rotarImagen(R1, R2, size_IM, 5);

    unsigned char* R3 = new unsigned char[size_IM];
    aplicarXOR(R2, I_M, R3, size_IM);

    exportImage(R3, width_IM, height_IM, "Reconstruida_Caso1.bmp");
    cout << "Reconstrucción del Caso 1 completada." << endl;
    delete[] I_M;
    delete[] I_O;
    delete[] M;
    delete[] P1;
    delete[] P2;
    delete[] P3;
    delete[] M2_txt;
    delete[] M1_txt;
    delete[] R1;
    delete[] R2;
    delete[] R3;
    return 0;
}



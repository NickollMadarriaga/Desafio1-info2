#include <QCoreApplication>
#include <QImage>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;

unsigned char* loadPixels(QString input, int &w, int &h);
unsigned int*   loadSeedMasking(const char* file, int &seed, int &n);
void            aplicarXOR(unsigned char* A, unsigned char* B, unsigned char* out, int size);
void            rotarImagen(unsigned char* in, unsigned char* out, int size, int bits);
bool            verificarEnmascaramiento(unsigned char* img, unsigned char* M, unsigned int* data, int seed, int n);
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

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    int width_IO = 0, height_IO = 0;
    unsigned char* IO = loadPixels("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\I_O.bmp", width_IO, height_IO);
    int size_IO = width_IO * height_IO * 3;
    int width_IM = 0, height_IM = 0;
    unsigned char* IM = loadPixels("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\I_M.bmp", width_IM, height_IM);
    int size_IM = width_IM * height_IM * 3;
    int width_M = 0, height_M = 0;
    unsigned char* M  = loadPixels("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M.bmp",  width_M, height_M);
    // int size_M = width_M * height_M * 3;

    // Paso 1: P1 = IO XOR IM
    unsigned char* P1 = new unsigned char[size_IO];
    cout <<"tamaño"<<size_IO<< endl;

    aplicarXOR(IO, IM, P1, size_IO);
    int seed1 = 0, n1 = 0;
    unsigned int* T1 = loadSeedMasking("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M1.txt", seed1, n1);
    if (!verificarEnmascaramiento(P1, M, T1, seed1, n1)){
        cout << "Error: M1.txt no coincide" << endl;
    }


    // Paso 2: Rotar P1 4 bits → P2
    unsigned char* P2 = new unsigned char[size_IO];
    int seed2 = 0, n2 = 0;
    unsigned int* T2 = loadSeedMasking("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M2.txt", seed2, n2);
    rotarImagen(P1, P2, size_IO, 4);
    if (!verificarEnmascaramiento(P2, M, T2, seed2, n2)){
        cout << "Error: M2.txt no coincide" << endl;
    }

    // Paso 3: P2 XOR IM → P3
    unsigned char* P3 = new unsigned char[size_IM];
    aplicarXOR(P2, IM, P3, size_IM);
    int seed3 = 0, n3 = 0;
    unsigned int* T3 = loadSeedMasking("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M3.txt", seed3, n3);
    if (!verificarEnmascaramiento(P3, M, T3, seed3, n3)){
        cout << "Error: M3.txt no coincide" << endl;
    return 0;
    }

    // Paso 4: Rotar P3 5 bits → P4
    unsigned char* P4 = new unsigned char[size_IM];
    rotarImagen(P3, P4, size_IM, 5);
    int seed4 = 0, n4 = 0;
    unsigned int* T4 = loadSeedMasking("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M4.txt", seed4, n4);
    if (!verificarEnmascaramiento(P4, M, T4, seed4, n4)){
        cout << "Error: M4.txt no coincide" << endl;
    return 0;
    }

    // Paso 5: P4 XOR IM → P5
    unsigned char* P5 = new unsigned char[size_IM];
    aplicarXOR(P4, IM, P5, size_IM);
    int seed5 = 0, n5 = 0;
    unsigned int* T5 = loadSeedMasking("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M5.txt", seed5, n5);
     if (!verificarEnmascaramiento(P5, M, T5, seed5, n5)) {
        cout << "Error: M5.txt no coincide" << endl; return 0;
    }

    // Paso 6: Rotar P5 2 bit → P6
    unsigned char* P6 = new unsigned char[size_IM];
    rotarImagen(P5, P6, size_IM, 2);
    int seed6 = 0, n6 = 0;
    unsigned int* T6 = loadSeedMasking("C:\\Users\\nicko\\OneDrive\\Documentos\\GitHub\\Desafio1-info2\\caso_2\\untitled2\\M6.txt", seed6, n6);
    if (!verificarEnmascaramiento(P6, M, T6, seed6, n6)) {
        cout << "Error: M6.txt no coincide" << endl; return 0;
    }

    unsigned char* R5 = new unsigned char[size_IM];
    rotarImagen(P6, R5, size_IM, 6);

    unsigned char* R4 = new unsigned char[size_IM];
    aplicarXOR(R5, IM, R4, size_IM);

    unsigned char* R3 = new unsigned char[size_IM];
    rotarImagen(R4, R3, size_IM, 3);

    unsigned char* R2 = new unsigned char[size_IM];
    aplicarXOR(R3, IM, R2, size_IM);

    unsigned char* R1 = new unsigned char[size_IO];
    rotarImagen(R2, R1, size_IO, 4);

    unsigned char* imagenReconstruida = new unsigned char[size_IO];
    aplicarXOR(R1, IM, imagenReconstruida, size_IO);


    exportImage(imagenReconstruida, width_IM, height_IM, "Reconstruida_Caso2.bmp");
    cout << "Imagen reconstruida correctamente." << endl;

    delete[] IO; delete[] IM; delete[] M;
    delete[] P1; delete[] P2; delete[] P3;
    delete[] P4; delete[] P5; delete[] P6;
    delete[] T1; delete[] T2; delete[] T3;
    delete[] T4; delete[] T5; delete[] T6;
    delete[] R5; delete[] R4; delete[] R3;
    delete[] R2; delete[] R1; delete[] imagenReconstruida;

    return 0;
}





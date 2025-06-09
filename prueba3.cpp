#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/freetype.hpp>
#include <cstdlib>     // For system()
#include <algorithm>   // For std::min and std::max
#include <cctype>      // For isspace
#include <sys/stat.h>  // For stat() to check directory existence

using namespace cv;
using namespace std;

const string FUENTE = "Montserrat-Bold.ttf"; // Asegúrate de que la fuente esté en el mismo directorio
const int IMG_WIDTH = 1920;
const int IMG_HEIGHT = 1080;

// Colores
const Scalar COLOR_RECTANGULO_CELESTE_AZULADO = Scalar(255, 175, 80); // Un celeste azulado (BGR: B=255, G=175, R=80)
const Scalar COLOR_TEXTO_BLANCO = Scalar(255, 255, 255); // Blanco (BGR)
const Scalar COLOR_TEXTO_OUTLINE_NEGRO = Scalar(0, 0, 0); // Negro para el contorno (BGR)

const double RECTANGLE_OPACITY = 0.75; // Opacidad del rectángulo

const int MARGIN_TOP = 30; // Margen desde la parte superior de la imagen (anteriormente MARGIN_BOTTOM)
const int MARGIN_SIDES = 30;  // Margen desde los lados izquierdo y derecho de la imagen
const int PADDING_VERTICAL = 30; // Relleno vertical para el texto dentro del rectángulo
const int PADDING_HORIZONTAL = 50; // Relleno horizontal para el texto dentro del rectángulo

// Tamaño de fuente para el texto principal
// Ajustado para el texto más largo "Escucha con subtítulos en Inglés y Español"
const int FONT_HEIGHT_MAIN_TEXT = 80    ; 

// Grosor del contorno (outline) del texto
const int OUTLINE_THICKNESS = 4; // Se mantiene en el valor original

// Función para eliminar espacios en blanco al principio y al final de una cadena
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

// Envuelve el texto en múltiples líneas para ajustarse al ancho máximo.
// Devuelve solo las líneas de texto.
vector<string> wrapText(Ptr<freetype::FreeType2> ft2, const string& text, int fontHeight, int maxWidth) {
    vector<string> lines;
    stringstream ss(text);
    string word;
    string currentLine;
    bool firstWordInLine = true;

    while (ss >> word) {
        string testLine = currentLine;
        if (!firstWordInLine) {
            testLine += " ";
        }
        testLine += word;

        Size textSize = ft2->getTextSize(testLine, fontHeight, -1, nullptr);

        if (textSize.width <= maxWidth) {
            currentLine = testLine;
        } else {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }
            currentLine = word;
            firstWordInLine = true; // Reiniciar para la nueva línea
        }
        firstWordInLine = false;
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    return lines;
}

// Calcula la altura total del texto envuelto.
int calculateWrappedTextHeight(Ptr<freetype::FreeType2> ft2, const string& text, int fontHeight, int maxWidth) {
    vector<string> lines = wrapText(ft2, text, fontHeight, maxWidth);
    if (lines.empty()) return 0;
    
    // Obtener la altura de una sola línea renderizada para cálculos precisos
    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    
    int totalTextHeight = lines.size() * singleLineRenderHeight;
    return totalTextHeight;
}

// Dibuja texto envuelto, centrado vertical y horizontalmente dentro de un Rect con un contorno.
void drawWrappedTextWithOutline(
    Mat& img,
    Ptr<freetype::FreeType2> ft2,
    const string& text,
    const Rect& rect,
    int fontHeight,
    Scalar textColor,
    Scalar outlineColor,
    int outlineThickness) { 
    
    // El área disponible para el texto es el ancho del rectángulo menos el padding horizontal
    // En este contexto, PADDING_HORIZONTAL y PADDING_VERTICAL son globales y se aplican.
    int text_area_width = rect.width - (2 * PADDING_HORIZONTAL);
    vector<string> lines = wrapText(ft2, text, fontHeight, text_area_width);

    if (lines.empty()) return;

    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    int totalRenderedTextHeight = lines.size() * singleLineRenderHeight;

    // Calcular la posición inicial Y para centrar el bloque de texto verticalmente dentro del rectángulo
    int current_line_y_start = rect.y + PADDING_VERTICAL + ( (rect.height - (2 * PADDING_VERTICAL) - totalRenderedTextHeight) / 2 );
    
    for (const string& line : lines) {
        Size lineSize = ft2->getTextSize(line, fontHeight, -1, nullptr);
        int line_x_initial = rect.x + PADDING_HORIZONTAL + (text_area_width - lineSize.width) / 2;
        int baseline_y_initial = current_line_y_start + singleLineRenderHeight;
        
        // Dibuja el contorno
        for (int i = -outlineThickness; i <= outlineThickness; ++i) {
            for (int j = -outlineThickness; j <= outlineThickness; ++j) {
                if (i == 0 && j == 0) continue; 
                ft2->putText(img, line, Point(line_x_initial + j, baseline_y_initial + i), fontHeight, outlineColor, -1, LINE_AA, true);
            }
        }
        // Dibuja el texto principal en el centro
        ft2->putText(img, line, Point(line_x_initial, baseline_y_initial), fontHeight, textColor, -1, LINE_AA, true);

        current_line_y_start += singleLineRenderHeight; // Mover a la siguiente línea
    }
}


int main() {
    // Cargar la imagen de fondo
    Mat backgroundImage = imread("personajes/1000.png");
    if (backgroundImage.empty()) {
        cerr << "Error: No se pudo cargar la imagen de fondo desde 'personajes/1000.png'. Asegurese de que la ruta sea correcta." << endl;
        return 1;
    }
    // Redimensionar a las dimensiones estándar si es necesario
    resize(backgroundImage, backgroundImage, Size(IMG_WIDTH, IMG_HEIGHT), 0, 0, INTER_LINEAR);

    // Inicializar FreeType para el texto
    Ptr<freetype::FreeType2> ft2 = freetype::createFreeType2();
    try {
        ft2->loadFontData(FUENTE, 0);
    } catch (const cv::Exception& e) {
        cerr << "Error: No se pudo cargar la fuente '" << FUENTE << "'. Asegurese de que este en el mismo directorio que el ejecutable." << endl;
        cerr << "Error de OpenCV FreeType: " << e.what() << endl;
        return 1;
    }
    
    string text_to_display = "Escucha con subtítulos en Inglés";

    // 1. Calcular el ancho del texto envuelto para determinar el ancho del rectángulo
    // El área disponible para el texto se calcula inicialmente con el ancho total de la imagen menos los márgenes laterales.
    int max_text_width_for_wrap = IMG_WIDTH - (2 * MARGIN_SIDES) - (2 * PADDING_HORIZONTAL);
    
    // wrapped_lines es necesario para calcular el ancho y alto del texto antes de dibujar
    vector<string> wrapped_lines = wrapText(ft2, text_to_display, FONT_HEIGHT_MAIN_TEXT, max_text_width_for_wrap);
    
    int actual_wrapped_text_width = 0;
    for (const string& line : wrapped_lines) {
        actual_wrapped_text_width = max(actual_wrapped_text_width, ft2->getTextSize(line, FONT_HEIGHT_MAIN_TEXT, -1, nullptr).width);
    }

    int text_height_in_lines = calculateWrappedTextHeight(ft2, text_to_display, FONT_HEIGHT_MAIN_TEXT, max_text_width_for_wrap);
    
    // 2. Calcular las dimensiones y posición del rectángulo
    int rect_width = actual_wrapped_text_width + (2 * PADDING_HORIZONTAL);
    int rect_height = text_height_in_lines + (2 * PADDING_VERTICAL);

    // Asegurarse de que el ancho del rectángulo no exceda el ancho de la imagen menos los márgenes
    rect_width = min(rect_width, IMG_WIDTH - (2 * MARGIN_SIDES));

    int rect_x = (IMG_WIDTH - rect_width) / 2; // Centrado horizontalmente
    // Posición del rectángulo en la parte superior, usando MARGIN_TOP
    int rect_y = MARGIN_TOP; 

    Rect mainRect(rect_x, rect_y, rect_width, rect_height);

    // Asegurarse de que el directorio de salida exista
    string output_dir = "imagenes_generadas";
    struct stat info;
    if (stat(output_dir.c_str(), &info) != 0) {
        int system_result = system(("mkdir " + output_dir).c_str());
        if (system_result != 0) {
            cerr << "Error: Fallo al crear el directorio de salida '" << output_dir << "'. Codigo de retorno: " << system_result << endl;
            return 1;
        }
    } else if (!(info.st_mode & S_IFDIR)) {
        cerr << "Error: La ruta '" << output_dir << "' existe pero no es un directorio." << endl;
        return 1;
    }

    // Crear la imagen final
    Mat outputImage = backgroundImage.clone();

    // Aplicar el rectángulo semi-transparente
    Mat roi = outputImage(mainRect);
    // Usar el color celeste azulado para el rectángulo
    Mat coloredOverlay(roi.size(), outputImage.type(), COLOR_RECTANGULO_CELESTE_AZULADO); 
    addWeighted(coloredOverlay, RECTANGLE_OPACITY, roi, 1.0 - RECTANGLE_OPACITY, 0.0, roi);

    // Dibujar el texto sobre el rectángulo con contorno
    drawWrappedTextWithOutline(outputImage, ft2, text_to_display, mainRect, FONT_HEIGHT_MAIN_TEXT, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS);

    // Guardar la imagen generada
    string output_filepath = output_dir + "/prueba_output3.png";
    imwrite(output_filepath, outputImage);
    cout << "✅ Imagen generada exitosamente: " << output_filepath << endl;

    cout << "Presiona cualquier tecla para cerrar esta ventana..." << endl;
    system("pause"); 

    return 0;
}

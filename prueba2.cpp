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
const Scalar COLOR_RECTANGULO_VERDE_TRANSPARENTE = Scalar(120, 255, 120); // Verde más claro (BGR: B=120, G=255, R=120)
const Scalar COLOR_TEXTO_BLANCO = Scalar(255, 255, 255); // Blanco (BGR)
const Scalar COLOR_TEXTO_OUTLINE_NEGRO = Scalar(0, 0, 0); // Negro para el contorno (BGR)

const double RECTANGLE_OPACITY = 0.70; // Opacidad de los rectángulos (reducida un poco)

const int MARGIN_BOTTOM = 30; // Margen desde la parte inferior de la imagen para el rectángulo inferior
const int MARGIN_SIDES = 30;  // Margen desde los lados izquierdo y derecho de la imagen

const int SPACING_BETWEEN_RECTS = 20; // Espacio vertical entre los dos rectángulos

// Parámetros para el rectángulo SUPERIOR (Azul)
const int FONT_HEIGHT_BLUE_TEXT = 110; // Aumentado ligeramente
const int PADDING_VERTICAL_BLUE = 30; // Relleno vertical para el texto azul
const int PADDING_HORIZONTAL_BLUE = 50; // Relleno horizontal para el texto azul

// Parámetros para el rectángulo INFERIOR (Verde)
const int FONT_HEIGHT_GREEN_TEXT = 90; // Aumentado considerablemente
const int PADDING_VERTICAL_GREEN = 15; // Relleno vertical para el texto verde (más pequeño)
const int PADDING_HORIZONTAL_GREEN = 25; // Relleno horizontal para el texto verde (más pequeño)

// Grosor del contorno (outline) del texto (común para ambos)
const int OUTLINE_THICKNESS = 5; // Aumentado ligeramente

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
// Ahora recibe el padding horizontal y vertical como argumentos para flexibilidad.
void drawWrappedTextWithOutline(
    Mat& img,
    Ptr<freetype::FreeType2> ft2,
    const string& text,
    const Rect& rect,
    int fontHeight,
    Scalar textColor,
    Scalar outlineColor,
    int outlineThickness,
    int horizontalPadding, // Padding horizontal para el texto dentro de este rectángulo
    int verticalPadding)   // Padding vertical para el texto dentro de este rectángulo
{ 
    // El área disponible para el texto es el ancho del rectángulo menos el padding horizontal
    int text_area_width = rect.width - (2 * horizontalPadding);
    vector<string> lines = wrapText(ft2, text, fontHeight, text_area_width);

    if (lines.empty()) return;

    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    int totalRenderedTextHeight = lines.size() * singleLineRenderHeight;

    // Calcular la posición inicial Y para centrar el bloque de texto verticalmente dentro del Rect
    int current_line_y_start = rect.y + verticalPadding + ( (rect.height - (2 * verticalPadding) - totalRenderedTextHeight) / 2 );
    
    for (const string& line : lines) {
        Size lineSize = ft2->getTextSize(line, fontHeight, -1, nullptr);
        int line_x_initial = rect.x + horizontalPadding + (text_area_width - lineSize.width) / 2; // Centrar la línea horizontalmente
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
    
    string text_blue_rect = "¿Sientes que has mejorado?";
    string text_green_rect = "Cuéntamelo en los comentarios";

    // --- CÁLCULO Y DIBUJO DEL RECTÁNGULO VERDE (INFERIOR) ---
    // Calcular el ancho del texto envuelto para determinar el ancho del rectángulo VERDE
    int max_text_width_green = IMG_WIDTH - (2 * MARGIN_SIDES) - (2 * PADDING_HORIZONTAL_GREEN);
    vector<string> wrapped_lines_green = wrapText(ft2, text_green_rect, FONT_HEIGHT_GREEN_TEXT, max_text_width_green);
    
    int actual_wrapped_text_width_green = 0;
    for (const string& line : wrapped_lines_green) {
        actual_wrapped_text_width_green = max(actual_wrapped_text_width_green, ft2->getTextSize(line, FONT_HEIGHT_GREEN_TEXT, -1, nullptr).width);
    }

    int text_height_in_lines_green = calculateWrappedTextHeight(ft2, text_green_rect, FONT_HEIGHT_GREEN_TEXT, max_text_width_green);
    
    int green_rect_width = actual_wrapped_text_width_green + (2 * PADDING_HORIZONTAL_GREEN);
    int green_rect_height = text_height_in_lines_green + (2 * PADDING_VERTICAL_GREEN); 

    green_rect_width = min(green_rect_width, IMG_WIDTH - (2 * MARGIN_SIDES)); // Asegurar que no exceda los márgenes

    int green_rect_x = (IMG_WIDTH - green_rect_width) / 2; // Centrado horizontalmente
    int green_rect_y = IMG_HEIGHT - green_rect_height - MARGIN_BOTTOM; // Posición basada en el margen inferior

    Rect greenRect(green_rect_x, green_rect_y, green_rect_width, green_rect_height);

    // --- CÁLCULO Y DIBUJO DEL RECTÁNGULO AZUL (SUPERIOR) ---
    // Calcular el ancho del texto envuelto para determinar el ancho del rectángulo AZUL
    int max_text_width_blue = IMG_WIDTH - (2 * MARGIN_SIDES) - (2 * PADDING_HORIZONTAL_BLUE);
    vector<string> wrapped_lines_blue = wrapText(ft2, text_blue_rect, FONT_HEIGHT_BLUE_TEXT, max_text_width_blue);
    
    int actual_wrapped_text_width_blue = 0;
    for (const string& line : wrapped_lines_blue) {
        actual_wrapped_text_width_blue = max(actual_wrapped_text_width_blue, ft2->getTextSize(line, FONT_HEIGHT_BLUE_TEXT, -1, nullptr).width);
    }

    int text_height_in_lines_blue = calculateWrappedTextHeight(ft2, text_blue_rect, FONT_HEIGHT_BLUE_TEXT, max_text_width_blue);
    
    int blue_rect_width = actual_wrapped_text_width_blue + (2 * PADDING_HORIZONTAL_BLUE);
    int blue_rect_height = text_height_in_lines_blue + (2 * PADDING_VERTICAL_BLUE);

    blue_rect_width = min(blue_rect_width, IMG_WIDTH - (2 * MARGIN_SIDES)); // Asegurar que no exceda los márgenes

    int blue_rect_x = (IMG_WIDTH - blue_rect_width) / 2; // Centrado horizontalmente
    // El rectángulo azul se posiciona encima del verde con el espacio definido
    int blue_rect_y = green_rect_y - SPACING_BETWEEN_RECTS - blue_rect_height; 

    Rect blueRect(blue_rect_x, blue_rect_y, blue_rect_width, blue_rect_height);


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

    // --- Aplicar los rectángulos semi-transparentes (esquinas vivas) ---
    // Dibujar primero el rectángulo verde (inferior)
    Mat roi_green = outputImage(greenRect);
    Mat coloredOverlay_green(roi_green.size(), outputImage.type(), COLOR_RECTANGULO_VERDE_TRANSPARENTE); 
    addWeighted(coloredOverlay_green, RECTANGLE_OPACITY, roi_green, 1.0 - RECTANGLE_OPACITY, 0.0, roi_green);

    // Dibujar luego el rectángulo azul (superior)
    Mat roi_blue = outputImage(blueRect);
    Mat coloredOverlay_blue(roi_blue.size(), outputImage.type(), COLOR_RECTANGULO_CELESTE_AZULADO); 
    addWeighted(coloredOverlay_blue, RECTANGLE_OPACITY, roi_blue, 1.0 - RECTANGLE_OPACITY, 0.0, roi_blue);

    // Dibujar el texto en el rectángulo azul con contorno
    drawWrappedTextWithOutline(outputImage, ft2, text_blue_rect, blueRect, FONT_HEIGHT_BLUE_TEXT, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS, PADDING_HORIZONTAL_BLUE, PADDING_VERTICAL_BLUE);

    // Dibujar el texto en el rectángulo verde con contorno
    drawWrappedTextWithOutline(outputImage, ft2, text_green_rect, greenRect, FONT_HEIGHT_GREEN_TEXT, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS, PADDING_HORIZONTAL_GREEN, PADDING_VERTICAL_GREEN);

    // Guardar la imagen generada
    string output_filepath = output_dir + "/prueba_output2.png";
    imwrite(output_filepath, outputImage);
    cout << "✅ Imagen generada exitosamente: " << output_filepath << endl;

    cout << "Presiona cualquier tecla para cerrar esta ventana..." << endl;
    system("pause"); 

    return 0;
}

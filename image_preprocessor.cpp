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
#include <filesystem>  // For std::filesystem (requires C++17)

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

// Structure to hold the parsed indices data from IndicesImagenes.txt
struct IndicesData {
    std::vector<int> english_only_images;
    std::vector<int> english_spanish_images;
    int total_phrases = 0;
    int total_generated_images = 0;
};

// --- CONSTANTES GLOBALES PARA GENERACION DE IMAGENES ---
const string FONT_PATH = "Montserrat-Bold.ttf"; 
const int BASE_IMG_WIDTH = 1920; 
const int BASE_IMG_HEIGHT = 1080; 

// Colores
const Scalar COLOR_RECTANGULO_CELESTE_AZULADO = Scalar(255, 175, 80); // BGR: B=255, G=175, R=80
const Scalar COLOR_RECTANGULO_VERDE_CLARO = Scalar(120, 255, 120); // BGR: B=120, G=255, R=120
const Scalar COLOR_TEXTO_BLANCO = Scalar(255, 255, 255); 
const Scalar COLOR_TEXTO_OUTLINE_NEGRO = Scalar(0, 0, 0); 

// Opacidades
const double RECTANGLE_OPACITY_LISTENING_SUBTITLES = 0.75; 
const double RECTANGLE_OPACITY_TEST = 0.70; 

// Márgenes y Espaciados
const int MARGIN_TOP_DEFAULT = 30;    // Para rectángulos en la parte superior
const int MARGIN_BOTTOM_DEFAULT = 30; // Para rectángulos en la parte inferior
const int MARGIN_SIDES_DEFAULT = 30;  // Para ambos lados

const int SPACING_BETWEEN_TEST_RECTS = 20; // Espacio entre los dos rectángulos de "Test"

// Paddings (relleno del texto dentro del rectángulo)
const int PADDING_VERTICAL_LISTENING = 30;   // Para "Escucha sin Subtítulos"
const int PADDING_HORIZONTAL_LISTENING = 50; 

const int PADDING_VERTICAL_SUBTITLES = 30;   // Para "Escucha con subtítulos..."
const int PADDING_HORIZONTAL_SUBTITLES = 50;

const int PADDING_VERTICAL_TEST_BLUE = 30;   // Para rect azul de "Test"
const int PADDING_HORIZONTAL_TEST_BLUE = 50;

const int PADDING_VERTICAL_TEST_GREEN = 15;  // Para rect verde de "Test"
const int PADDING_HORIZONTAL_TEST_GREEN = 25;

// Tamaños de Fuente
const int FONT_HEIGHT_LISTENING = 120;       // "Escucha sin Subtítulos"
const int FONT_HEIGHT_SUBTITLES_EN_ES = 75;  // "Escucha con subtítulos en Inglés y Español"
const int FONT_HEIGHT_SUBTITLES_EN = 80;     // "Escucha con subtítulos en Inglés"
const int FONT_HEIGHT_TEST_BLUE = 100;       // "¿Sientes que has mejorado?"
const int FONT_HEIGHT_TEST_GREEN = 70;       // "Cuéntamelo en los comentarios"

// Grosor del contorno (outline) del texto
const int OUTLINE_THICKNESS = 4; 

// --- FIN CONSTANTES GLOBALES ---


// Function to read and parse IndicesImagenes.txt
IndicesData read_indices_file(const std::string& filename) {
    IndicesData data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        line_num++;
        std::stringstream ss(line);
        std::string segment;

        if (line_num == 1) { // English only images
            while (std::getline(ss, segment, ',')) {
                try {
                    data.english_only_images.push_back(std::stoi(segment));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Formato invalido en la linea 1 de " << filename << ". Esperaba numeros." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        } else if (line_num == 2) { // English and Spanish images
            while (std::getline(ss, segment, ',')) {
                try {
                    data.english_spanish_images.push_back(std::stoi(segment));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Formato invalido en la linea 2 de " << filename << ". Esperaba numeros." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        } else if (line_num == 3) { // Total phrases
            try {
                data.total_phrases = std::stoi(line);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Formato invalido en la linea 3 de " << filename << ". Esperaba un numero entero." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (line_num == 4) { // Total generated images
            try {
                data.total_generated_images = std::stoi(line);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Formato invalido en la linea 4 de " << filename << ". Esperaba un numero entero." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    if (line_num < 4) {
        std::cerr << "Error: El archivo " << filename << " no tiene el formato esperado (menos de 4 lineas)." << std::endl;
        exit(EXIT_FAILURE);
    }

    return data;
}


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
            firstWordInLine = true; 
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
    int outlineThickness,
    int horizontalPadding, 
    int verticalPadding)   
{ 
    int text_area_width = rect.width - (2 * horizontalPadding);
    vector<string> lines = wrapText(ft2, text, fontHeight, text_area_width);

    if (lines.empty()) return;

    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    int totalRenderedTextHeight = lines.size() * singleLineRenderHeight;

    int current_line_y_start = rect.y + verticalPadding + ( (rect.height - (2 * verticalPadding) - totalRenderedTextHeight) / 2 );
    
    for (const string& line : lines) {
        Size lineSize = ft2->getTextSize(line, fontHeight, -1, nullptr);
        int line_x_initial = rect.x + horizontalPadding + (text_area_width - lineSize.width) / 2;
        int baseline_y_initial = current_line_y_start + singleLineRenderHeight;
        
        for (int i = -outlineThickness; i <= outlineThickness; ++i) {
            for (int j = -outlineThickness; j <= outlineThickness; ++j) {
                if (i == 0 && j == 0) continue; 
                ft2->putText(img, line, Point(line_x_initial + j, baseline_y_initial + i), fontHeight, outlineColor, -1, LINE_AA, true);
            }
        }
        ft2->putText(img, line, Point(line_x_initial, baseline_y_initial), fontHeight, textColor, -1, LINE_AA, true);

        current_line_y_start += singleLineRenderHeight;
    }
}

// Function to generate the "Listening" image (Fondo Sin Subtitulos style)
void generate_listening_image(const std::string& base_image_path, const std::string& output_filepath) {
    Mat backgroundImage = imread(base_image_path);
    if (backgroundImage.empty()) {
        cerr << "Error: No se pudo cargar la imagen base desde '" << base_image_path << "' para Listening Image." << endl;
        exit(EXIT_FAILURE);
    }
    resize(backgroundImage, backgroundImage, Size(BASE_IMG_WIDTH, BASE_IMG_HEIGHT), 0, 0, INTER_LINEAR);

    Ptr<freetype::FreeType2> ft2 = freetype::createFreeType2();
    try {
        ft2->loadFontData(FONT_PATH, 0);
    } catch (const cv::Exception& e) {
        cerr << "Error: No se pudo cargar la fuente '" << FONT_PATH << "'. Asegurese de que este en el mismo directorio que el ejecutable." << endl;
        cerr << "Error de OpenCV FreeType: " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
    
    string text_to_display = "Escucha sin Subtítulos";

    int max_text_width_for_wrap = BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT) - (2 * PADDING_HORIZONTAL_LISTENING);
    vector<string> wrapped_lines = wrapText(ft2, text_to_display, FONT_HEIGHT_LISTENING, max_text_width_for_wrap);
    
    int actual_wrapped_text_width = 0;
    for (const string& line : wrapped_lines) {
        actual_wrapped_text_width = max(actual_wrapped_text_width, ft2->getTextSize(line, FONT_HEIGHT_LISTENING, -1, nullptr).width);
    }

    int text_height_in_lines = calculateWrappedTextHeight(ft2, text_to_display, FONT_HEIGHT_LISTENING, max_text_width_for_wrap);
    
    int rect_width = actual_wrapped_text_width + (2 * PADDING_HORIZONTAL_LISTENING);
    int rect_height = text_height_in_lines + (2 * PADDING_VERTICAL_LISTENING);

    rect_width = min(rect_width, BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT));

    int rect_x = (BASE_IMG_WIDTH - rect_width) / 2; // Centrado horizontalmente
    int rect_y = BASE_IMG_HEIGHT - rect_height - MARGIN_BOTTOM_DEFAULT; // Posición en la parte inferior

    Rect mainRect(rect_x, rect_y, rect_width, rect_height);

    Mat outputImage = backgroundImage.clone();

    // Aplicar el rectángulo semi-transparente
    Mat roi = outputImage(mainRect);
    Mat coloredOverlay(roi.size(), outputImage.type(), COLOR_RECTANGULO_CELESTE_AZULADO); 
    addWeighted(coloredOverlay, RECTANGLE_OPACITY_LISTENING_SUBTITLES, roi, 1.0 - RECTANGLE_OPACITY_LISTENING_SUBTITLES, 0.0, roi);

    // Dibujar el texto sobre el rectángulo con contorno
    drawWrappedTextWithOutline(outputImage, ft2, text_to_display, mainRect, FONT_HEIGHT_LISTENING, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS, PADDING_HORIZONTAL_LISTENING, PADDING_VERTICAL_LISTENING);

    imwrite(output_filepath, outputImage);
    cout << "✅ Imagen generada: " << output_filepath << endl;
}

// Function to generate the "Test" image (Fondo con Test style)
void generate_test_image(const std::string& base_image_path, const std::string& output_filepath) {
    Mat backgroundImage = imread(base_image_path);
    if (backgroundImage.empty()) {
        cerr << "Error: No se pudo cargar la imagen base desde '" << base_image_path << "' para Test Image." << endl;
        exit(EXIT_FAILURE);
    }
    resize(backgroundImage, backgroundImage, Size(BASE_IMG_WIDTH, BASE_IMG_HEIGHT), 0, 0, INTER_LINEAR);

    Ptr<freetype::FreeType2> ft2 = freetype::createFreeType2();
    try {
        ft2->loadFontData(FONT_PATH, 0);
    } catch (const cv::Exception& e) {
        cerr << "Error: No se pudo cargar la fuente '" << FONT_PATH << "'. Asegurese de que este en el mismo directorio que el ejecutable." << endl;
        cerr << "Error de OpenCV FreeType: " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
    
    string text_blue_rect = "¿Sientes que has mejorado?";
    string text_green_rect = "Cuéntamelo en los comentarios";

    // --- CÁLCULO Y DIBUJO DEL RECTÁNGULO VERDE (INFERIOR) ---
    int max_text_width_green = BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT) - (2 * PADDING_HORIZONTAL_TEST_GREEN);
    vector<string> wrapped_lines_green = wrapText(ft2, text_green_rect, FONT_HEIGHT_TEST_GREEN, max_text_width_green);
    
    int actual_wrapped_text_width_green = 0;
    for (const string& line : wrapped_lines_green) {
        actual_wrapped_text_width_green = max(actual_wrapped_text_width_green, ft2->getTextSize(line, FONT_HEIGHT_TEST_GREEN, -1, nullptr).width);
    }

    int text_height_in_lines_green = calculateWrappedTextHeight(ft2, text_green_rect, FONT_HEIGHT_TEST_GREEN, max_text_width_green);
    
    int green_rect_width = actual_wrapped_text_width_green + (2 * PADDING_HORIZONTAL_TEST_GREEN);
    int green_rect_height = text_height_in_lines_green + (2 * PADDING_VERTICAL_TEST_GREEN);

    green_rect_width = min(green_rect_width, BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT));

    int green_rect_x = (BASE_IMG_WIDTH - green_rect_width) / 2;
    int green_rect_y = BASE_IMG_HEIGHT - green_rect_height - MARGIN_BOTTOM_DEFAULT; 

    Rect greenRect(green_rect_x, green_rect_y, green_rect_width, green_rect_height);

    // --- CÁLCULO Y DIBUJO DEL RECTÁNGULO AZUL (SUPERIOR) ---
    int max_text_width_blue = BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT) - (2 * PADDING_HORIZONTAL_TEST_BLUE);
    vector<string> wrapped_lines_blue = wrapText(ft2, text_blue_rect, FONT_HEIGHT_TEST_BLUE, max_text_width_blue);
    
    int actual_wrapped_text_width_blue = 0;
    for (const string& line : wrapped_lines_blue) {
        actual_wrapped_text_width_blue = max(actual_wrapped_text_width_blue, ft2->getTextSize(line, FONT_HEIGHT_TEST_BLUE, -1, nullptr).width);
    }

    int text_height_in_lines_blue = calculateWrappedTextHeight(ft2, text_blue_rect, FONT_HEIGHT_TEST_BLUE, max_text_width_blue);
    
    int blue_rect_width = actual_wrapped_text_width_blue + (2 * PADDING_HORIZONTAL_TEST_BLUE);
    int blue_rect_height = text_height_in_lines_blue + (2 * PADDING_VERTICAL_TEST_BLUE);

    blue_rect_width = min(blue_rect_width, BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT));

    int blue_rect_x = (BASE_IMG_WIDTH - blue_rect_width) / 2;
    int blue_rect_y = green_rect_y - SPACING_BETWEEN_TEST_RECTS - blue_rect_height; 

    Rect blueRect(blue_rect_x, blue_rect_y, blue_rect_width, blue_rect_height);

    Mat outputImage = backgroundImage.clone();

    // Aplicar los rectángulos semi-transparentes (esquinas vivas)
    Mat roi_green = outputImage(greenRect);
    Mat coloredOverlay_green(roi_green.size(), outputImage.type(), COLOR_RECTANGULO_VERDE_CLARO); 
    addWeighted(coloredOverlay_green, RECTANGLE_OPACITY_TEST, roi_green, 1.0 - RECTANGLE_OPACITY_TEST, 0.0, roi_green);

    Mat roi_blue = outputImage(blueRect);
    Mat coloredOverlay_blue(roi_blue.size(), outputImage.type(), COLOR_RECTANGULO_CELESTE_AZULADO); 
    addWeighted(coloredOverlay_blue, RECTANGLE_OPACITY_TEST, roi_blue, 1.0 - RECTANGLE_OPACITY_TEST, 0.0, roi_blue);

    // Dibujar el texto en los rectángulos con contorno
    drawWrappedTextWithOutline(outputImage, ft2, text_blue_rect, blueRect, FONT_HEIGHT_TEST_BLUE, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS, PADDING_HORIZONTAL_TEST_BLUE, PADDING_VERTICAL_TEST_BLUE);
    drawWrappedTextWithOutline(outputImage, ft2, text_green_rect, greenRect, FONT_HEIGHT_TEST_GREEN, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS, PADDING_HORIZONTAL_TEST_GREEN, PADDING_VERTICAL_TEST_GREEN);

    imwrite(output_filepath, outputImage);
    cout << "✅ Imagen generada: " << output_filepath << endl;
}

// Function to overlay subtitle text onto an existing image (for English/Spanish subtitles)
void overlay_subtitle_text_image(const std::string& base_image_path, const std::string& output_filepath, const std::string& text_content, int font_height) {
    Mat backgroundImage = imread(base_image_path);
    if (backgroundImage.empty()) {
        cerr << "Error: No se pudo cargar la imagen base desde '" << base_image_path << "' para Subtitle Overlay." << endl;
        exit(EXIT_FAILURE);
    }
    resize(backgroundImage, backgroundImage, Size(BASE_IMG_WIDTH, BASE_IMG_HEIGHT), 0, 0, INTER_LINEAR);

    Ptr<freetype::FreeType2> ft2 = freetype::createFreeType2();
    try {
        ft2->loadFontData(FONT_PATH, 0);
    } catch (const cv::Exception& e) {
        cerr << "Error: No se pudo cargar la fuente '" << FONT_PATH << "'. Asegurese de que este en el mismo directorio que el ejecutable." << endl;
        cerr << "Error de OpenCV FreeType: " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
    
    int max_text_width_for_wrap = BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT) - (2 * PADDING_HORIZONTAL_SUBTITLES);
    vector<string> wrapped_lines = wrapText(ft2, text_content, font_height, max_text_width_for_wrap);
    
    int actual_wrapped_text_width = 0;
    for (const string& line : wrapped_lines) {
        actual_wrapped_text_width = max(actual_wrapped_text_width, ft2->getTextSize(line, font_height, -1, nullptr).width);
    }

    int text_height_in_lines = calculateWrappedTextHeight(ft2, text_content, font_height, max_text_width_for_wrap);
    
    int rect_width = actual_wrapped_text_width + (2 * PADDING_HORIZONTAL_SUBTITLES);
    int rect_height = text_height_in_lines + (2 * PADDING_VERTICAL_SUBTITLES);

    rect_width = min(rect_width, BASE_IMG_WIDTH - (2 * MARGIN_SIDES_DEFAULT));

    int rect_x = (BASE_IMG_WIDTH - rect_width) / 2; // Centrado horizontalmente
    int rect_y = MARGIN_TOP_DEFAULT; // Posición en la parte superior

    Rect mainRect(rect_x, rect_y, rect_width, rect_height);

    Mat outputImage = backgroundImage.clone();

    // Aplicar el rectángulo semi-transparente
    Mat roi = outputImage(mainRect);
    Mat coloredOverlay(roi.size(), outputImage.type(), COLOR_RECTANGULO_CELESTE_AZULADO); 
    addWeighted(coloredOverlay, RECTANGLE_OPACITY_LISTENING_SUBTITLES, roi, 1.0 - RECTANGLE_OPACITY_LISTENING_SUBTITLES, 0.0, roi);

    // Dibujar el texto sobre el rectángulo con contorno
    drawWrappedTextWithOutline(outputImage, ft2, text_content, mainRect, font_height, COLOR_TEXTO_BLANCO, COLOR_TEXTO_OUTLINE_NEGRO, OUTLINE_THICKNESS, PADDING_HORIZONTAL_SUBTITLES, PADDING_VERTICAL_SUBTITLES);

    imwrite(output_filepath, outputImage);
    cout << "✅ Imagen generada: " << output_filepath << endl;
}


int main() {
    // Read indices from file
    IndicesData indices = read_indices_file("IndicesImagenes.txt");

    // Asegurarse de que los directorios de salida existan
    fs::create_directories("imagenes_generadas");
    fs::create_directories("Imagenes_English");
    fs::create_directories("Imagenes_Spanish");

    // --- Generar imágenes "Listening" (Fondo Sin Subtítulos) ---
    cout << "\nGenerando imagenes de Listening (Fondo Sin Subtitulos)..." << endl;
    generate_listening_image("personajes/1000.png", "personajes/1000_Listening.png");
    generate_listening_image("personajes/2000.png", "personajes/2000_Listening.png");

    // --- Generar imágenes "Test" (Fondo con Test) ---
    cout << "\nGenerando imagenes de Test (Fondo con Test)..." << endl;
    generate_test_image("personajes/1000.png", "personajes/1000_Test.png");
    generate_test_image("personajes/2000.png", "personajes/2000_Test.png");

    // --- Generar imágenes para "Fondo con subtitulos en inglés" ---
    cout << "\nGenerando imagenes para Fondo con subtitulos en ingles..." << endl;
    for (int img_idx : indices.english_only_images) {
        string source_img_path = "imagenes_generadas/" + std::to_string(img_idx) + ".png";
        string output_img_path = "Imagenes_English/" + std::to_string(img_idx) + ".png";
        // Necesitas asegurarte de que imagenes_generadas/{index}.png exista antes de esto.
        // Si no existen, este programa los saltará o dará error.
        // Para la demo, asumimos que ya existen o se generarán por otro lado.
        // Si no es así, esta parte deberá ser parte de un flujo más amplio.
        if (fs::exists(source_img_path)) {
            overlay_subtitle_text_image(source_img_path, output_img_path, "Escucha con subtítulos en Inglés", FONT_HEIGHT_SUBTITLES_EN);
        } else {
            cerr << "Advertencia: La imagen original " << source_img_path << " no existe. No se puede generar la imagen para subtítulos en ingles." << endl;
        }
    }

    // --- Generar imágenes para "Fondo con subtitulos en y español" ---
    cout << "\nGenerando imagenes para Fondo con subtitulos en ingles y espanol..." << endl;
    for (int img_idx : indices.english_spanish_images) {
        string source_img_path = "imagenes_generadas/" + std::to_string(img_idx) + ".png";
        string output_img_path = "Imagenes_Spanish/" + std::to_string(img_idx) + ".png";
        if (fs::exists(source_img_path)) {
            overlay_subtitle_text_image(source_img_path, output_img_path, "Escucha con subtítulos en Inglés y Español", FONT_HEIGHT_SUBTITLES_EN_ES);
        } else {
            cerr << "Advertencia: La imagen original " << source_img_path << " no existe. No se puede generar la imagen para subtítulos en ingles y espanol." << endl;
        }
    }

    cout << "\nProceso de preprocesamiento de imagenes completado." << endl;
 

    return EXIT_SUCCESS;
}

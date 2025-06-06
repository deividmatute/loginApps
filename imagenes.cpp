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
#include <cstdlib>   // For system()
#include <algorithm> // For std::min and std::max
#include <cctype>    // For isspace
#include <sys/stat.h> // For stat() to check directory existence

using namespace cv;
using namespace std;

const string FUENTE = "Montserrat-Bold.ttf"; // Ensure the font is in the same folder
const int IMG_WIDTH = 1920;
const int IMG_HEIGHT = 1080;

// Colors
const Scalar COLOR_RECTANGULO_NUEVO = Scalar(25, 25, 25);     // Near Black for the semi-transparent rectangle
const Scalar COLOR_TEXTO_INGLES_NUEVO = Scalar(89, 222, 255); // Yellow (#ffde59) -> BGR
const Scalar COLOR_TEXTO_SUBFRASE_NUEVO = Scalar(99, 191, 0); // Green (#00bf63) -> BGR
const Scalar COLOR_TEXTO_ESPANOL_NUEVO = Scalar(255, 255, 255); // White (BGR)

const string ARCHIVO_PLANTILLA = "plantilla.txt";
const int LINE_SPACING = 40;
const int RECT_VERTICAL_PADDING = 40;

const int TOP_TEXT_OFFSET_FRAGMENTO = 20;
const int BOTTOM_TEXT_OFFSET_ESPANOL = -20;
const double RECTANGLE_OPACITY = 0.85; // Opacity for the main rectangle (0.0 fully transparent, 1.0 fully opaque) - Changed to 0.85


// Function to remove leading and trailing whitespace from a string
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

// Draws a rectangle with sharp corners (radius parameter is kept for compatibility but forced to 0 logic)
// This function might not be directly used for the main semi-transparent rectangle anymore,
// as blending is handled separately. It's kept if other solid rectangles were needed.
void drawSharpRect(Mat& img, Rect rect, Scalar color) {
    rectangle(img, rect, color, -1, LINE_AA); // -1 for filled
}


// Wraps text into multiple lines to fit within maxWidth.
// Returns only the lines of text.
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
            firstWordInLine = true; // Reset for the new line
        }
        firstWordInLine = false;
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    return lines;
}

// Wraps text and returns lines with their original start indices.
vector<pair<string, int>> wrapTextAndOriginalIndices(Ptr<freetype::FreeType2> ft2, const string& text, int fontHeight, int maxWidth) {
    vector<pair<string, int>> lines_with_indices;
    string currentLineContent;
    int currentLineOriginalStartIndex = 0;

    vector<pair<string, int>> words_with_original_indices;
    size_t current_char_pos = 0;
    while (current_char_pos < text.length()) {
        while (current_char_pos < text.length() && isspace(text[current_char_pos])) {
            current_char_pos++;
        }
        if (current_char_pos == text.length()) break;

        size_t word_start = current_char_pos;
        while (current_char_pos < text.length() && !isspace(text[current_char_pos])) {
            current_char_pos++;
        }
        string word = text.substr(word_start, current_char_pos - word_start);
        words_with_original_indices.push_back({word, static_cast<int>(word_start)});
    }

    if (words_with_original_indices.empty()) {
        return lines_with_indices;
    }

    currentLineContent = words_with_original_indices[0].first;
    currentLineOriginalStartIndex = words_with_original_indices[0].second;

    for (size_t i = 1; i < words_with_original_indices.size(); ++i) {
        string nextWord = words_with_original_indices[i].first;
        int nextWordOriginalIndex = words_with_original_indices[i].second;

        string testLine = currentLineContent;
        if (!currentLineContent.empty()) {
            testLine += " ";
        }
        testLine += nextWord;

        Size textSize = ft2->getTextSize(testLine, fontHeight, -1, nullptr);

        if (textSize.width <= maxWidth) {
            currentLineContent = testLine;
        } else {
            lines_with_indices.push_back({currentLineContent, currentLineOriginalStartIndex});
            currentLineContent = nextWord;
            currentLineOriginalStartIndex = nextWordOriginalIndex;
        }
    }
    lines_with_indices.push_back({currentLineContent, currentLineOriginalStartIndex});
    return lines_with_indices;
}

// Calculates the total height of wrapped text.
int calculateWrappedTextHeight(Ptr<freetype::FreeType2> ft2, const string& text, int fontHeight, int maxWidth) {
    vector<string> lines = wrapText(ft2, text, fontHeight, maxWidth);
    if (lines.empty()) return 0;
    
    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    
    int totalTextHeight = lines.size() * singleLineRenderHeight;
    if (lines.size() > 1) {
        totalTextHeight += (lines.size() - 1) * LINE_SPACING; // Space between lines
    }
    return totalTextHeight;
}

// Draws wrapped text, centered within the rect, with a vertical offset.
void drawWrappedText(
    Mat& img,
    Ptr<freetype::FreeType2> ft2,
    const string& text,
    const Rect& rect,
    int fontHeight,
    Scalar color,
    int y_offset = 0) {
    int text_area_width = static_cast<int>(rect.width * 0.95);
    vector<string> lines = wrapText(ft2, text, fontHeight, text_area_width);

    if (lines.empty()) return;

    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    int totalRenderedTextHeight = lines.size() * singleLineRenderHeight;
    if (lines.size() > 1) {
        totalRenderedTextHeight += (lines.size() - 1) * LINE_SPACING;
    }
    
    int current_line_y_start = rect.y + (rect.height - totalRenderedTextHeight) / 2 + y_offset;

    for (const string& line : lines) {
        Size lineSize = ft2->getTextSize(line, fontHeight, -1, nullptr);
        int text_area_start_x = rect.x + (rect.width - text_area_width) / 2;
        int line_x = text_area_start_x + (text_area_width - lineSize.width) / 2;
        int baseline_y = current_line_y_start + singleLineRenderHeight;
        
        ft2->putText(img, line, Point(line_x, baseline_y), fontHeight, color, -1, LINE_AA, true);
        current_line_y_start += singleLineRenderHeight + LINE_SPACING;
    }
}

// Draws wrapped text with a highlighted fragment.
void drawWrappedTextWithHighlight(
    Mat& img,
    Ptr<freetype::FreeType2> ft2,
    const string& fullText,
    const string& highlightText,
    const Rect& rect,
    int fontHeight,
    Scalar defaultColor,
    Scalar highlightColor,
    int y_offset = 0) {
    int text_area_width = static_cast<int>(rect.width * 0.95);
    vector<pair<string, int>> lines_with_indices = wrapTextAndOriginalIndices(ft2, fullText, fontHeight, text_area_width);

    if (lines_with_indices.empty()) return;

    int singleLineRenderHeight = ft2->getTextSize("Tg", fontHeight, -1, nullptr).height;
    int totalRenderedTextHeight = lines_with_indices.size() * singleLineRenderHeight;
    if (lines_with_indices.size() > 1) { // Corrected: changed 'lines.size()' to 'lines_with_indices.size()'
        totalRenderedTextHeight += (lines_with_indices.size() - 1) * LINE_SPACING;
    }

    int current_line_y_start = rect.y + (rect.height - totalRenderedTextHeight) / 2 + y_offset;

    size_t highlight_start_in_fullText = fullText.find(highlightText);
    size_t highlight_end_in_fullText = (highlight_start_in_fullText == string::npos) ? 0 : highlight_start_in_fullText + highlightText.length();

    if (highlight_start_in_fullText == string::npos) {
        drawWrappedText(img, ft2, fullText, rect, fontHeight, defaultColor, y_offset);
        return;
    }

    int text_area_start_x = rect.x + (rect.width - text_area_width) / 2;

    for (const auto& line_pair : lines_with_indices) {
        const string& line_content = line_pair.first;
        int line_original_start_index = line_pair.second;

        Size lineSize = ft2->getTextSize(line_content, fontHeight, -1, nullptr);
        int line_x_initial = text_area_start_x + (text_area_width - lineSize.width) / 2;
        int baseline_y = current_line_y_start + singleLineRenderHeight;

        int current_draw_x = line_x_initial;
        int current_char_in_line_index = 0;

        while (current_char_in_line_index < line_content.length()) {
            int char_original_index_in_fullText = line_original_start_index + current_char_in_line_index;
            bool is_highlighted_segment = (char_original_index_in_fullText >= highlight_start_in_fullText && char_original_index_in_fullText < highlight_end_in_fullText);

            int segment_end_in_line_index = current_char_in_line_index;
            while (segment_end_in_line_index < line_content.length()) {
                int next_char_original_index_in_fullText = line_original_start_index + segment_end_in_line_index;
                bool next_is_highlighted_segment = (next_char_original_index_in_fullText >= highlight_start_in_fullText && next_char_original_index_in_fullText < highlight_end_in_fullText);
                if (is_highlighted_segment != next_is_highlighted_segment) break;
                segment_end_in_line_index++;
            }

            string segment_to_draw = line_content.substr(current_char_in_line_index, segment_end_in_line_index - current_char_in_line_index);
            Scalar drawColor = is_highlighted_segment ? highlightColor : defaultColor;

            ft2->putText(img, segment_to_draw, Point(current_draw_x, baseline_y), fontHeight, drawColor, -1, LINE_AA, true);
            current_draw_x += ft2->getTextSize(segment_to_draw, fontHeight, -1, nullptr).width;
            current_char_in_line_index = segment_end_in_line_index;
        }
        current_line_y_start += singleLineRenderHeight + LINE_SPACING;
    }
}

int main() {
    // Define font path (constant)
    const string FUENTE = "Montserrat-Bold.ttf"; // Ensure the font is in the same folder

    // Initialize background image path index
    int background_image_idx = 0; // 0 for 1000.png, 1 for 2000.png

    vector<vector<string>> frases_data;
    ifstream archivo(ARCHIVO_PLANTILLA);
    if (archivo.is_open()) {
        string linea;
        while (getline(archivo, linea)) {
            stringstream ss(linea);
            string fragmento;
            vector<string> linea_data;
            while (getline(ss, fragmento, '|')) {
                linea_data.push_back(trim(fragmento));
            }
            if (linea_data.size() >= 2) {
                frases_data.push_back(linea_data);
            }
        }
        archivo.close();
    } else {
        cerr << "Error: No se pudo abrir el archivo de plantilla: " << ARCHIVO_PLANTILLA << endl;
        return 1;
    }

    string output_dir = "imagenes_generadas";
    // Check if directory exists before attempting to create to avoid error messages if it already exists
    struct stat info;
    if (stat(output_dir.c_str(), &info) != 0) {
        // Directory does not exist, try to create
        int system_result = system(("mkdir " + output_dir).c_str());
        if (system_result != 0) {
            cerr << "Error: Fallo al crear el directorio de salida '" << output_dir << "'. Codigo de retorno: " << system_result << endl;
            // Proceeding anyway, but this could cause issues if directory doesn't exist
        }
    } else if (!(info.st_mode & S_IFDIR)) {
        // Path exists but is not a directory
        cerr << "Error: La ruta '" << output_dir << "' existe pero no es un directorio." << endl;
        // Proceeding anyway, but this could cause issues
    }


    Ptr<freetype::FreeType2> ft2 = freetype::createFreeType2();
    // Check if font loading was successful
    try {
        ft2->loadFontData(FUENTE, 0);
    } catch (const cv::Exception& e) {
        cerr << "Error: No se pudo cargar la fuente '" << FUENTE << "'. Asegurese de que este en el mismo directorio que el ejecutable." << endl;
        cerr << "Error de OpenCV FreeType: " << e.what() << endl;
        return 1;
    }
    

    int spacing = 20; 

    const int MIN_HEIGHT_EN_SECTION = 120;
    const int MIN_HEIGHT_ES_SECTION = 80;
    const int HEIGHT_FRAGMENTO_ES_SECTION = 80;

    int fontHeight_en = static_cast<int>(75 * 1.15);
    int fontHeight_es = static_cast<int>(55 * 1.15);
    int fontHeight_fragmento_es = static_cast<int>(50 * 1.15);

    int contador_imagenes = 1; // Start at 1 for image naming
    vector<int> imagenes_ingles_solo;
    vector<int> imagenes_ingles_y_espanol;

    int main_rect_width = IMG_WIDTH;
    int main_rect_x = (IMG_WIDTH - main_rect_width) / 2; // Will be 0

    if (frases_data.empty()) {
        cout << "No se encontraron frases en plantilla.txt. No se generaran imagenes." << endl;
    }

    // --- Image Generation Loop ---
    for (const auto& frase_data : frases_data) {
        // Determine current background image path
        string current_background_image_path;
        if (background_image_idx == 0) {
            current_background_image_path = "personajes/1000.png";
        } else {
            current_background_image_path = "personajes/2000.png";
        }
        background_image_idx = 1 - background_image_idx; // Toggle for next phrase

        // Load the background image for the current phrase
        Mat backgroundImage = imread(current_background_image_path);
        if (backgroundImage.empty()) {
            cerr << "Error: No se pudo cargar la imagen de fondo desde " << current_background_image_path << endl;
            cerr << "Asegurese de que la carpeta 'imagenes' exista y contenga '" 
                 << (background_image_idx == 0 ? "2000.png" : "1000.png") // Show the missing image
                 << "' en relacion con el ejecutable." << endl;
            return 1; // Exit if background image for current phrase cannot be loaded
        }
        resize(backgroundImage, backgroundImage, Size(IMG_WIDTH, IMG_HEIGHT), 0, 0, INTER_LINEAR);


        string frase_en = frase_data[0];
        string frase_es = frase_data[1];
        vector<pair<string, string>> subfrases;
        for (size_t i = 2; i + 1 < frase_data.size(); i += 2) {
            subfrases.push_back({frase_data[i], frase_data[i + 1]});
        }

        int effective_text_content_width = static_cast<int>(main_rect_width * 0.95);

        int required_height_fragmento_es_content = calculateWrappedTextHeight(ft2, subfrases.empty() ? "" : subfrases[0].second, fontHeight_fragmento_es, effective_text_content_width);
        int required_height_en_content = calculateWrappedTextHeight(ft2, frase_en, fontHeight_en, effective_text_content_width);
        int required_height_es_content = calculateWrappedTextHeight(ft2, frase_es, fontHeight_es, effective_text_content_width);

        int actual_height_fragmento_es_section = max(HEIGHT_FRAGMENTO_ES_SECTION, required_height_fragmento_es_content + RECT_VERTICAL_PADDING);
        int actual_height_en_section = max(MIN_HEIGHT_EN_SECTION, required_height_en_content + RECT_VERTICAL_PADDING);
        int actual_height_es_section = max(MIN_HEIGHT_ES_SECTION, required_height_es_content + RECT_VERTICAL_PADDING);
        
        int total_main_rect_height = actual_height_fragmento_es_section + spacing + actual_height_en_section + spacing + actual_height_es_section;
        int main_rect_y = IMG_HEIGHT - total_main_rect_height;
        Rect mainRect(main_rect_x, main_rect_y, main_rect_width, total_main_rect_height);


        // Helper lambda to apply the semi-transparent rectangle
        auto applySemiTransparentRect = [&](Mat& targetImage, const Rect& rectToOverlay) {
            if (rectToOverlay.width <= 0 || rectToOverlay.height <= 0) return;
            Mat roi = targetImage(rectToOverlay);
            Mat coloredOverlay(roi.size(), targetImage.type(), COLOR_RECTANGULO_NUEVO);
            addWeighted(coloredOverlay, RECTANGLE_OPACITY, roi, 1.0 - RECTANGLE_OPACITY, 0.0, roi);
        };

        // Image 1: Background with semi-transparent rectangle
        Mat img1 = backgroundImage.clone();
        applySemiTransparentRect(img1, mainRect);
        imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img1);
        cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
        contador_imagenes++;


        // Image 2: English phrase
        Mat img2 = backgroundImage.clone();
        applySemiTransparentRect(img2, mainRect);
        Rect rect_en_section_img2(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing, mainRect.width, actual_height_en_section);
        drawWrappedText(img2, ft2, frase_en, rect_en_section_img2, fontHeight_en, COLOR_TEXTO_INGLES_NUEVO);
        imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img2);
        imagenes_ingles_solo.push_back(contador_imagenes);
        cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
        contador_imagenes++;


        // Image 3: English and Spanish phrases
        Mat img3 = backgroundImage.clone();
        applySemiTransparentRect(img3, mainRect);
        Rect rect_en_section_img3(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing, mainRect.width, actual_height_en_section);
        drawWrappedText(img3, ft2, frase_en, rect_en_section_img3, fontHeight_en, COLOR_TEXTO_INGLES_NUEVO);
        Rect rect_es_section_img3(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing + actual_height_en_section + spacing, mainRect.width, actual_height_es_section);
        drawWrappedText(img3, ft2, frase_es, rect_es_section_img3, fontHeight_es, COLOR_TEXTO_ESPANOL_NUEVO, BOTTOM_TEXT_OFFSET_ESPANOL);
        imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img3);
        imagenes_ingles_y_espanol.push_back(contador_imagenes);
        cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
        contador_imagenes++;


        for (const auto& subfrase : subfrases) {
            Mat img_fragmento = backgroundImage.clone();
            applySemiTransparentRect(img_fragmento, mainRect);

            Rect rect_fragmento_es(mainRect.x, mainRect.y, mainRect.width, actual_height_fragmento_es_section);
            drawWrappedText(img_fragmento, ft2, subfrase.second, rect_fragmento_es, fontHeight_fragmento_es, COLOR_TEXTO_SUBFRASE_NUEVO, TOP_TEXT_OFFSET_FRAGMENTO);

            Rect rect_en_highlight_section(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing, mainRect.width, actual_height_en_section);
            drawWrappedTextWithHighlight(img_fragmento, ft2, frase_en, subfrase.first, rect_en_highlight_section, fontHeight_en, COLOR_TEXTO_INGLES_NUEVO, COLOR_TEXTO_SUBFRASE_NUEVO);
            
            Rect rect_es_section(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing + actual_height_en_section + spacing, mainRect.width, actual_height_es_section);
            drawWrappedText(img_fragmento, ft2, frase_es, rect_es_section, fontHeight_es, COLOR_TEXTO_ESPANOL_NUEVO, BOTTOM_TEXT_OFFSET_ESPANOL);

            imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img_fragmento);
            cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
            contador_imagenes++;

            imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img_fragmento); // Repeat
            cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
            contador_imagenes++;
        }

        Mat img_final = backgroundImage.clone();
        applySemiTransparentRect(img_final, mainRect);
        Rect rect_en_final_section(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing, mainRect.width, actual_height_en_section);
        drawWrappedText(img_final, ft2, frase_en, rect_en_final_section, fontHeight_en, COLOR_TEXTO_INGLES_NUEVO);
        Rect rect_es_final_section(mainRect.x, mainRect.y + actual_height_fragmento_es_section + spacing + actual_height_en_section + spacing, mainRect.width, actual_height_es_section);
        drawWrappedText(img_final, ft2, frase_es, rect_es_final_section, fontHeight_es, COLOR_TEXTO_ESPANOL_NUEVO, BOTTOM_TEXT_OFFSET_ESPANOL);
        imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img_final);
        cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
        contador_imagenes++;

        imwrite(output_dir + "/" + to_string(contador_imagenes) + ".png", img_final); // Repeat
        cout << "✅ Imagen generada: " << output_dir + "/" + to_string(contador_imagenes) + ".png" << endl;
        contador_imagenes++;
    }

    cout << "\n✨ Todas las imagenes han sido generadas en la carpeta: " << output_dir << endl;

    // Save lists and counts to IndicesImagenes.txt
    ofstream indices_file("IndicesImagenes.txt", ios::trunc); // Open in truncate mode to clear existing content
    if (indices_file.is_open()) {
        // Lista de imagenes solo en ingles
        for (size_t i = 0; i < imagenes_ingles_solo.size(); ++i) {
            indices_file << imagenes_ingles_solo[i] << (i == imagenes_ingles_solo.size() - 1 ? "" : ",");
        }
        indices_file << endl;

        // Lista de imagenes en ingles y espanol
        for (size_t i = 0; i < imagenes_ingles_y_espanol.size(); ++i) {
            indices_file << imagenes_ingles_y_espanol[i] << (i == imagenes_ingles_y_espanol.size() - 1 ? "" : ",");
        }
        indices_file << endl;

        // Total de frases procesadas
        indices_file << frases_data.size() << endl;
        
        // Total de imagenes generadas
        indices_file << contador_imagenes - 1 << endl; // contador_imagenes is 1 past the last image number

        indices_file.close();
        cout << "✅ Indices de imagenes guardados en IndicesImagenes.txt" << endl;
    } else {
        cerr << "Error: No se pudo abrir el archivo IndicesImagenes.txt para escritura." << endl;
    }

    cout << "Presiona cualquier tecla para cerrar esta ventana..." << endl;
    system("pause"); // This will print "Press any key to continue . . ." and wait for input

    return 0;
}

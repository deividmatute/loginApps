#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <regex>
#include <filesystem>
#include <numeric>
#include <memory>

namespace fs = std::filesystem;

// Structure to hold the parsed indices data from IndicesImagenes.txt
struct IndicesData {
    std::vector<int> english_only_images;
    std::vector<int> english_spanish_images;
    int total_phrases = 0;
    int total_generated_images = 0;
};

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

// Helper class to manage temporary files, ensuring they are deleted on exit
class TempFile {
    std::string filename;
public:
    explicit TempFile(const std::string& name) : filename(name) {}
    ~TempFile() {
        try { if (!filename.empty()) fs::remove(filename); } catch (...) {}
    }
    std::string path() const { return filename; }
};

// Lists files in a directory matching a regex pattern and sorts them numerically
std::vector<std::string> list_files_sorted(const std::string& path, const std::string& pattern) {
    std::vector<std::string> files;
    std::regex re(pattern, std::regex_constants::icase);

    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "Error: El directorio " << path << " no existe o no es un directorio." << std::endl;
            exit(EXIT_FAILURE);
        }
        for (const auto& entry : fs::directory_iterator(path)) {
            std::string filename = entry.path().filename().string();
            if (std::regex_match(filename, re)) {
                files.push_back(entry.path().string());
            }
        }
        std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
            auto extract_number = [](const std::string& s) -> int {
                std::smatch match;
                if (std::regex_search(s, match, std::regex(R"(\d+)")) ) {
                    return std::stoi(match[0].str());
                }
                return 0;
            };
            return extract_number(a) < extract_number(b);
        });
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al leer el directorio: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    // Replace backslashes with forward slashes for FFmpeg compatibility
    for (auto& file : files) {
        std::replace(file.begin(), file.end(), '\\', '/');
    }
    return files;
}

// Executes a shell command and checks for errors
void exec_command(const std::string& cmd) {
    std::cout << "Ejecutando: " << cmd << std::endl; // Print the command
    int ret = std::system(cmd.c_str()); // Execute without redirection to see all output
    if (ret != 0) {
        std::cerr << "Error al ejecutar el comando. Codigo de salida: " << ret << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Creates a silence audio file using FFmpeg
void create_silence_file(const std::string& silence_file, float duration) {
    std::cout << "Creando archivo de silencio: " << silence_file << std::endl;
    std::string cmd = "ffmpeg -y -f lavfi -i anullsrc=r=44100:cl=mono -t " + std::to_string(duration) + " \"" + silence_file + "\"";
    exec_command(cmd);
}

// Gets the duration of an audio file using FFprobe
float get_audio_duration(const std::string& audio_file) {
    // ffprobe -v error already sends errors to stderr. Redirect stdout to NUL.
    // This popen call specifically captures stdout for duration, while silencing stderr.
    std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + audio_file + "\"";
    
    FILE* pipe = popen((cmd + " 2>NUL").c_str(), "r"); // Redirect stderr to NUL for popen
    if (!pipe) {
        std::cerr << "Error al obtener la duracion del audio" << std::endl;
        exit(EXIT_FAILURE);
    }
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    try {
        return std::stof(result);
    } catch (...) {
        std::cerr << "Error al convertir la duracion: " << result << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Concatenates an audio file with a silence file
void create_audio_with_silence(const std::string& audio, const std::string& silence, const std::string& output) {
    std::string cmd = "ffmpeg -y -i \"" + audio + "\" -i \"" + silence + "\" -filter_complex \"[0][1]concat=n=2:v=0:a=1[out]\" -map \"[out]\" \"" + output + "\"";
    exec_command(cmd);
}

int main() {
    // Read indices from file
    IndicesData indices = read_indices_file("IndicesImagenes.txt");

    std::string video_type_choice, language_choice;
    float silence_duration;
    std::string video_name;

    std::vector<std::string> audios_to_process;
    std::vector<std::string> images_to_process;

    std::cout << "Que tipo de video deseas generar?\n1. Dialogo\n2. Main Lesson\nOpcion (1/2): ";
    std::getline(std::cin, video_type_choice);

    if (video_type_choice == "1") { // Dialogo
        silence_duration = 1.0f;
        std::cout << "Idioma del dialogo?\n1. Ingles\n2. Espanol\n3. Fondo\nOpcion (1/2/3): ";
        std::getline(std::cin, language_choice);

        const std::string dialogue_audio_base_path = "Audios/Frases_English"; // All dialogue options use this audio path
        auto all_dialogue_audios = list_files_sorted(dialogue_audio_base_path, R"(en\d+\.mp3)");

        if (language_choice == "1") { // Dialogo -> Ingles
            video_name = "Dialogo_English.mp4";
            for (int img_idx : indices.english_only_images) {
                images_to_process.push_back("imagenes_generadas/" + std::to_string(img_idx) + ".png");
            }
            // Populate audios_to_process based on the count of images
            for(size_t i = 0; i < images_to_process.size(); ++i) {
                if (i < all_dialogue_audios.size()) {
                    audios_to_process.push_back(all_dialogue_audios[i]);
                } else {
                    std::cerr << "Advertencia: No hay suficientes archivos de audio para todas las imagenes de dialogo en ingles. Se usaran los audios disponibles." << std::endl;
                    // If not enough audios, use what's available and adjust image count
                    images_to_process.resize(all_dialogue_audios.size());
                    audios_to_process = all_dialogue_audios;
                    break; 
                }
            }
            if (audios_to_process.empty() && !images_to_process.empty()) { // Handle case where no audios found but images expected
                 std::cerr << "Error: No se encontraron audios para la opcion de dialogo en ingles." << std::endl;
                 return EXIT_FAILURE;
            }


        } else if (language_choice == "2") { // Dialogo -> Espanol
            video_name = "Dialogo_Spanish.mp4";
            for (int img_idx : indices.english_spanish_images) {
                images_to_process.push_back("imagenes_generadas/" + std::to_string(img_idx) + ".png");
            }
            // Populate audios_to_process based on the count of images
            for(size_t i = 0; i < images_to_process.size(); ++i) {
                if (i < all_dialogue_audios.size()) {
                    audios_to_process.push_back(all_dialogue_audios[i]);
                } else {
                    std::cerr << "Advertencia: No hay suficientes archivos de audio para todas las imagenes de dialogo en espanol. Se usaran los audios disponibles." << std::endl;
                    images_to_process.resize(all_dialogue_audios.size());
                    audios_to_process = all_dialogue_audios;
                    break;
                }
            }
            if (audios_to_process.empty() && !images_to_process.empty()) {
                 std::cerr << "Error: No se encontraron audios para la opcion de dialogo en espanol." << std::endl;
                 return EXIT_FAILURE;
            }

        } else if (language_choice == "3") { // Dialogo -> Fondo
            video_name = "Dialogo_Fondo.mp4";
            const std::string personajes_path = "personajes";
            for (int i = 0; i < indices.total_phrases; ++i) {
                if (i % 2 == 0) { // Alternates 1000.png and 2000.png
                    images_to_process.push_back(personajes_path + "/1000.png");
                } else {
                    images_to_process.push_back(personajes_path + "/2000.png");
                }
            }
            // Audios for Fondo option will also be from Frases_English, matching total_phrases
            for(int i = 0; i < indices.total_phrases; ++i) {
                if (i < all_dialogue_audios.size()) {
                    audios_to_process.push_back(all_dialogue_audios[i]);
                } else {
                    std::cerr << "Advertencia: No hay suficientes archivos de audio para todas las frases de dialogo de fondo. Se usaran los audios disponibles." << std::endl;
                    images_to_process.resize(all_dialogue_audios.size());
                    audios_to_process = all_dialogue_audios;
                    break;
                }
            }
            if (audios_to_process.empty() && !images_to_process.empty()) {
                 std::cerr << "Error: No se encontraron audios para la opcion de dialogo de fondo." << std::endl;
                 return EXIT_FAILURE;
            }

        } else {
            std::cerr << "Opcion de idioma/fondo invalida." << std::endl;
            return EXIT_FAILURE;
        }

    } else if (video_type_choice == "2") { // Main Lesson
        silence_duration = 2.0f;
        video_name = "Main_Lesson.mp4";
        const std::string main_lesson_audio_base_path = "Audios_Main_Lesson";
        audios_to_process = list_files_sorted(main_lesson_audio_base_path, R"(en\d+\.mp3)");
        
        // Images for Main Lesson are 1.png to total_generated_images.png
        for (int i = 1; i <= indices.total_generated_images; ++i) {
            images_to_process.push_back("imagenes_generadas/" + std::to_string(i) + ".png");
        }
        if (audios_to_process.empty() && !images_to_process.empty()) {
             std::cerr << "Error: No se encontraron audios para la opcion Main Lesson." << std::endl;
             return EXIT_FAILURE;
        }

    } else {
        std::cerr << "Opcion de tipo de video invalida." << std::endl;
        return EXIT_FAILURE;
    }

    // Validate if audio and image counts match
    if (images_to_process.size() != audios_to_process.size()) {
        std::cerr << "Error: La cantidad de imagenes (" << images_to_process.size() 
                  << ") no coincide con la cantidad de audios (" << audios_to_process.size() << ")." << std::endl;
        // If counts don't match, trim the longer vector to match the shorter one
        if (images_to_process.size() > audios_to_process.size()) {
            images_to_process.resize(audios_to_process.size());
            std::cerr << "Ajustando el numero de imagenes a " << images_to_process.size() << std::endl;
        } else {
            audios_to_process.resize(images_to_process.size());
            std::cerr << "Ajustando el numero de audios a " << audios_to_process.size() << std::endl;
        }
        // If after adjustment, one is still empty and the other is not, it's an issue
        if (images_to_process.empty() || audios_to_process.empty()) {
            std::cerr << "Error: Despues del ajuste, no hay suficientes imagenes o audios para crear el video." << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    // Ensure there's something to process
    if (images_to_process.empty() || audios_to_process.empty()) {
        std::cerr << "Error: No hay imagenes o audios para procesar. Verifique sus directorios y el archivo IndicesImagenes.txt" << std::endl;
        return EXIT_FAILURE;
    }


    const std::string output_audio_dir = "Audios_Generados"; // Changed from src/audiosGenerados
    const std::string output_video_dir = "Videos_Generados"; // Changed from src/videosGenerados
    const std::string final_output_audio_path = output_audio_dir + "/dialogo.mp3";
    const std::string final_output_video_path = output_video_dir + "/" + video_name;
    const std::string silence_file = "silence.mp3"; // Temporary silence file

    fs::create_directories(output_audio_dir);
    fs::create_directories(output_video_dir);

    // Remove old silence file if it exists
    if (fs::exists(silence_file)) fs::remove(silence_file);
    create_silence_file(silence_file, silence_duration);

    std::vector<std::string> bloques_audio_temp;
    for (size_t i = 0; i < audios_to_process.size(); ++i) {
        std::string bloque = "temp_audio_" + std::to_string(i) + ".mp3";
        create_audio_with_silence(audios_to_process[i], silence_file, bloque);
        bloques_audio_temp.push_back(bloque);
    }

    std::cout << "\nConcatenando audios..." << std::endl;
    TempFile list_audio("audio_list.txt");
    {
        std::ofstream out(list_audio.path());

        // Add a small initial silence
        const std::string silence_inicio = "inicio_silencio.mp3";
        if (fs::exists(silence_inicio)) fs::remove(silence_inicio); // Ensure clean slate
        create_silence_file(silence_inicio, 0.7f);
        out << "file '" << silence_inicio << "'\n";

        for (const auto& bloque : bloques_audio_temp) {
            out << "file '" << bloque << "'\n";
        }
    }
    // Added -y flag here to automatically overwrite the output audio file
    exec_command("ffmpeg -y -f concat -safe 0 -i " + list_audio.path() + " -c copy \"" + final_output_audio_path + "\"");

    std::cout << "\nPreparando lista de imagenes..." << std::endl;
    TempFile list_images("images_list.txt");
    {
        std::ofstream img_out(list_images.path());
        // The number of images should correspond to the number of audio blocks
        for (size_t i = 0; i < bloques_audio_temp.size(); ++i) {
            float duration = get_audio_duration(bloques_audio_temp[i]);
            // Ensure the image path exists before adding to list
            if (!fs::exists(images_to_process[i])) {
                std::cerr << "Error: La imagen " << images_to_process[i] << " no existe. Asegurese de que las imagenes esten generadas y en la ruta correcta." << std::endl;
                return EXIT_FAILURE;
            }
            img_out << "file '" << images_to_process[i] << "'\n";
            img_out << "duration " << duration << "\n";
        }
        // Add the last image again for the final duration, as FFmpeg's concat filter needs it.
        // This ensures the last image stays on screen for the duration of its corresponding audio.
        if (!images_to_process.empty()) { // Check if vector is not empty before accessing .back()
            img_out << "file '" << images_to_process.back() << "'\n";
        }
    }

    std::cout << "\nGenerando video..." << std::endl;
    std::string final_cmd = "ffmpeg -y -f concat -safe 0 -i " + list_images.path() +
                             " -i \"" + final_output_audio_path +
                             "\" -map 0:v:0 -map 1:a:0 -c:v libx264 -preset fast -crf 22 -pix_fmt yuv420p -c:a aac -shortest \"" + final_output_video_path + "\"";

    exec_command(final_cmd);
    
    std::cout << "\nEliminando archivos temporales de audio..." << std::endl;
    int temp_deleted = 0;
    for (const auto& archivo : fs::directory_iterator(".")) {
        std::string nombre = archivo.path().filename().string();
        if (std::regex_match(nombre, std::regex(R"(temp_audio_\d+\.mp3)"))) {
            fs::remove(archivo.path());
            ++temp_deleted;
        }
    }
    // Also remove the initial silence file
    if (fs::exists("inicio_silencio.mp3")) {
        fs::remove("inicio_silencio.mp3");
        ++temp_deleted;
    }
    // Also remove the silence.mp3 file
    if (fs::exists("silence.mp3")) {
        fs::remove("silence.mp3");
        ++temp_deleted;
    }
    std::cout << "Se eliminaron " << temp_deleted << " archivos temporales.\n";
    std::cout << "\nVideo generado exitosamente: " << final_output_video_path << std::endl;
    
    std::cout << "Presiona cualquier tecla para cerrar esta ventana..." << std::endl;
    system("pause"); 

    return EXIT_SUCCESS;
}

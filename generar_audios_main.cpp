#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <regex>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

// Ejecutar comandos con verificacion
void exec_command(const std::string& cmd) {
    std::cout << "Ejecutando: " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "❌ Error al ejecutar comando. Codigo de salida: " << ret << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Crear archivo de silencio si no existe
void crear_silencio(const std::string& archivo, float duracion) {
    // Always overwrite to ensure it's the correct duration if called multiple times
    std::string cmd = "ffmpeg -y -f lavfi -i anullsrc=r=44100:cl=mono -t " + std::to_string(duracion) + " \"" + archivo + "\"";
    exec_command(cmd);
}

// Extraer numeros de nombre de archivo
int extraer_numero(const std::string& nombre) {
    std::smatch match;
    if (std::regex_search(nombre, match, std::regex(R"(\d+)"))) {
        return std::stoi(match[0]);
    }
    return -1;
}

// Listar archivos en orden numerico
std::vector<fs::path> listar_archivos(const std::string& carpeta, const std::string& regex_str) {
    std::vector<fs::path> archivos;
    std::regex re(regex_str, std::regex::icase);
    if (!fs::exists(carpeta) || !fs::is_directory(carpeta)) {
        std::cerr << "Error: El directorio " << carpeta << " no existe o no es un directorio." << std::endl;
        exit(EXIT_FAILURE);
    }
    for (const auto& entry : fs::directory_iterator(carpeta)) {
        if (std::regex_match(entry.path().filename().string(), re)) {
            archivos.push_back(entry.path());
        }
    }
    std::sort(archivos.begin(), archivos.end(), [](const fs::path& a, const fs::path& b) {
        return extraer_numero(a.filename().string()) < extraer_numero(b.filename().string());
    });
    return archivos;
}

// Function to prepare a target directory: remove it if it exists, then create it.
void prepare_target_directory(const fs::path& target_dir_path) {
    if (fs::exists(target_dir_path)) {
        std::cout << "Limpiando directorio existente: " << target_dir_path.string() << "\n";
        try {
            fs::remove_all(target_dir_path);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error al limpiar el directorio " << target_dir_path.string() << ": " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }
    }
    try {
        fs::create_directories(target_dir_path);
        std::cout << "Directorio preparado: " << target_dir_path.string() << "\n";
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al crear el directorio " << target_dir_path.string() << ": " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}


int main() {
    std::string base_audios_conv = "Audios/";
    std::string en_dir = base_audios_conv + "Frases_English";
    std::string es_dir = base_audios_conv + "Frases_Spanish";
    std::string sub_dir_base = base_audios_conv + "SubFrases_Frase"; // Renamed for clarity
    std::string salida = "Audios_Main_Lesson";
    std::string silencio_file = "silence.mp3"; // Renamed for clarity
    float duracion_silencio = 0.5f; // A short silence for placeholders

    // Prepare the output directory (clean it if it exists)
    prepare_target_directory(salida);
    
    // Create the silence file (always overwrite)
    crear_silencio(silencio_file, duracion_silencio);

    auto frases_english_audios = listar_archivos(en_dir, R"(en\d+\.mp3)");
    int contador_salida_audios = 1; // This will count from 1 to 255

    for (const auto& frase_path : frases_english_audios) {
        int num = extraer_numero(frase_path.filename().string());
        std::string en_audio_path = en_dir + "/en" + std::to_string(num) + ".mp3";
        std::string es_audio_path = es_dir + "/es" + std::to_string(num) + ".mp3";
        std::string current_sub_frase_dir = sub_dir_base + std::to_string(num);

        // Logic to match image generation from 'imagenes.cpp'

        // 1. Corresponds to Image 1 (Background only) - Use silence as placeholder audio
        fs::copy(silencio_file, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);

        // 2. Corresponds to Image 2 (English phrase)
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);

        // 3. Corresponds to Image 3 (English + Spanish phrase)
        if (fs::exists(es_audio_path)) {
            fs::copy(es_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
        } else {
            // If Spanish audio doesn't exist, use English audio as fallback for this image slot
            fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
        }

        // 4. Corresponds to Subphrases (each generates 2 images: highlight + repeat)
        if (fs::exists(current_sub_frase_dir)) {
            auto subfrases_audios = listar_archivos(current_sub_frase_dir, R"(en\d+fr\d+\.mp3)"); // Regex for subphrases
            for (const auto& sub_audio_path : subfrases_audios) {
                // First subphrase image
                fs::copy(sub_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
                // Second subphrase (repeat) image
                fs::copy(sub_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
            }
        }

        // 5. Corresponds to Final English phrase (generates 2 images: final + repeat)
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
    }

    std::cout << "\n✅ Archivos preparados exitosamente en: " << salida << std::endl;
    std::cout << "Total de archivos generados: " << contador_salida_audios - 1 << std::endl; // Adjust for 1-based counting
    
    std::cout << "Presiona cualquier tecla para cerrar esta ventana..." << std::endl;
    system("pause"); 

    return 0;
}

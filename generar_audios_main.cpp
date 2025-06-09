#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include <sstream> // Necesario para std::stringstream en extraer_numero si se usa directamente

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
    // Regex para encontrar uno o más dígitos
    if (std::regex_search(nombre, match, std::regex(R"(\d+)"))) {
        return std::stoi(match[0]);
    }
    return -1; // Retorna -1 si no se encuentra ningún número
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
    std::string sub_dir_base = base_audios_conv + "SubFrases_Frase";
    std::string salida = "Audios_Main_Lesson"; // Directorio donde se generarán los audios finales
    std::string silencio_file = "silence.mp3"; // Archivo de silencio temporal

    // Preparar el directorio de salida (limpiarlo si existe)
    prepare_target_directory(salida);
    
    // Crear el archivo de silencio (siempre sobrescribir)
    // Usamos una duración pequeña, ya que este silencio es un 'relleno' en la lógica de copia.
    crear_silencio(silencio_file, 0.5f); 

    auto frases_english_audios = listar_archivos(en_dir, R"(en\d+\.mp3)");
    int contador_salida_audios = 1; // Contador para nombrar los archivos de salida (en1.mp3, en2.mp3, etc.)

    if (frases_english_audios.empty()) {
        std::cerr << "Error: No se encontraron audios base en '" << en_dir << "'. Asegurese de que los archivos 'enX.mp3' existan." << std::endl;
        return EXIT_FAILURE;
    }

    for (const auto& frase_path : frases_english_audios) {
        int num = extraer_numero(frase_path.filename().string());
        std::string en_audio_path = en_dir + "/en" + std::to_string(num) + ".mp3";
        std::string es_audio_path = es_dir + "/es" + std::to_string(num) + ".mp3";
        std::string current_sub_frase_dir = sub_dir_base + std::to_string(num);

        // --- Lógica de preparación de audios para Main Lesson ---
        // Se corrige el problema: los dos primeros audios deben ser la frase en inglés.

        // 1. Primera repetición de la frase en inglés
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);

        // 2. Segunda repetición de la frase en inglés
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);

        // 3. Traducción al español (o fallback a inglés si no existe)
        if (fs::exists(es_audio_path)) {
            fs::copy(es_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
        } else {
            std::cerr << "Advertencia: Audio en espanol para 'en" << num << ".mp3' no encontrado en '" << es_dir << "'. Usando la frase en ingles como respaldo.\n";
            fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
        }

        // 4. Subfrases (cada una se copia dos veces)
        if (fs::exists(current_sub_frase_dir) && fs::is_directory(current_sub_frase_dir)) {
            auto subfrases_audios = listar_archivos(current_sub_frase_dir, R"(en\d+fr\d+\.mp3)"); // Regex para subfrases
            for (const auto& sub_audio_path : subfrases_audios) {
                fs::copy(sub_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
                fs::copy(sub_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
            }
        } else {
             std::cout << "Info: Directorio de subfrases '" << current_sub_frase_dir << "' no encontrado o no es un directorio. No se agregaran subfrases para la frase en" << num << ".\n";
        }

        // 5. Frase final en inglés (se copia dos veces)
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
        fs::copy(en_audio_path, salida + "/en" + std::to_string(contador_salida_audios++) + ".mp3", fs::copy_options::overwrite_existing);
    }

    std::cout << "\n✅ Archivos preparados exitosamente en: " << salida << std::endl;
    std::cout << "Total de archivos generados: " << contador_salida_audios - 1 << std::endl; // Ajustar por el conteo basado en 1
    
    // Eliminar el archivo de silencio temporal si aún existe
    if (fs::exists(silencio_file)) {
        try {
            fs::remove(silencio_file);
            std::cout << "Se eliminó el archivo de silencio temporal: " << silencio_file << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error al eliminar el archivo de silencio temporal " << silencio_file << ": " << e.what() << "\n";
        }
    }

    return 0;
}

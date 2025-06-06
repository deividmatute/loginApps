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
#include <chrono>

namespace fs = std::filesystem;

// Constantes
const std::string RUTA_BASE_AUDIOS = "Audios_Sin_Nombres";
const std::string RUTA_DESTINO_AUDIOS = RUTA_BASE_AUDIOS + "/../Audios";

// Función para dividir una cadena por comas en un vector de enteros
std::vector<int> dividir_por_coma(const std::string& input) {
    std::vector<int> resultado;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        try {
            resultado.push_back(std::stoi(item));
        } catch (...) {
            std::cerr << "❌ Valor no valido: " << item << "\n";
        }
    }
    return resultado;
}

// Clase para gestionar archivos temporales
class TempFile {
    std::string filename;
public:
    explicit TempFile(const std::string& name) : filename(name) {}
    ~TempFile() {
        try { if (!filename.empty()) fs::remove(filename); } catch (...) {}
    }
    std::string path() const { return filename; }
};

// Función para preparar un directorio
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

// Función para extraer la marca de tiempo de nombres de archivo ElevenLabs
std::string extract_timestamp_from_elevenlabs_filename(const fs::path& filepath) {
    std::string filename = filepath.filename().string();
    std::regex timestamp_regex(R"(ElevenLabs_(\d{4}-\d{2}-\d{2}T\d{2}_\d{2}_\d{2}))");
    std::smatch matches;
    if (std::regex_search(filename, matches, timestamp_regex) && matches.size() > 1) {
        return matches[1].str();
    }
    return "";
}

// Función para verificar si un archivo ya ha sido renombrado a un formato objetivo
bool is_already_renamed(const fs::path& filepath, const std::string& prefix, bool is_fragment_mode) {
    std::string filename = filepath.filename().string();
    if (extract_timestamp_from_elevenlabs_filename(filepath).empty()) { // Solo aplica a archivos que NO son ElevenLabs originales
        if (is_fragment_mode) {
            // Ejemplo: en1fr1.mp3, es2fr3.mp3
            std::regex fragment_regex(prefix + R"(\d+fr\d+\.mp3)");
            return std::regex_match(filename, fragment_regex);
        } else {
            // Ejemplo: es1.mp3, en2.mp3
            std::regex normal_regex(prefix + R"(\d+\.mp3)");
            return std::regex_match(filename, normal_regex);
        }
    }
    return false;
}

// Función central para procesar cada carpeta
int process_audio_folder(
    const fs::path& source_folder,
    const fs::path& destination_base_folder,
    const std::string& prefix,
    bool is_odd_numbered,
    const std::vector<int>& fragments_list_for_this_folder,
    int& total_renamed_files_counter // Contador global para el total de archivos procesados
) {
    std::cout << "\nProcesando carpeta: " << source_folder.string() << "\n";
    std::vector<fs::path> mp3_files_to_process_paths; // Guardar paths directamente
    int files_already_renamed_count = 0;

    if (!fs::exists(source_folder)) {
        std::cerr << "❌ La ruta no existe: " << source_folder.string() << "\n";
        return 0;
    }

    // Recopilar archivos, filtrando los ya renombrados o simplemente copiándolos
    for (const auto& entry : fs::directory_iterator(source_folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
            bool is_fragment_mode = !fragments_list_for_this_folder.empty();
            if (is_already_renamed(entry.path(), prefix, is_fragment_mode)) {
                std::cout << "Archivo ya renombrado, copiando directamente: " << entry.path().filename().string() << "\n";
                fs::path final_destination_path;

                if (is_fragment_mode) {
                    std::regex fragment_name_regex(prefix + R"((\d+)fr\d+\.mp3)");
                    std::smatch matches;
                    std::string filename_str = entry.path().filename().string();
                    if (std::regex_search(filename_str, matches, fragment_name_regex) && matches.size() > 1) {
                        int frase_num = std::stoi(matches[1].str());
                        fs::path subfrases_target = destination_base_folder / ("SubFrases_Frase" + std::to_string(frase_num));
                        fs::create_directories(subfrases_target); // Asegurar que la carpeta exista
                        final_destination_path = subfrases_target / entry.path().filename();
                    } else {
                         std::cerr << "⚠️ Advertencia: Archivo fragmento pre-renombrado con formato inesperado: " << filename_str << ". Saltando.\n";
                         continue;
                    }
                } else {
                    final_destination_path = destination_base_folder / entry.path().filename();
                }

                try {
                    fs::copy(entry.path(), final_destination_path, fs::copy_options::overwrite_existing);
                    std::cout << "✅ Copiado a: " << final_destination_path.string() << "\n";
                    files_already_renamed_count++;
                } catch (const std::exception& e) {
                    std::cerr << "❌ Error al copiar archivo pre-renombrado: " << entry.path().filename().string() << " -> " << e.what() << "\n";
                }
            } else {
                mp3_files_to_process_paths.push_back(entry.path());
            }
        }
    }

    // Ordenar archivos de ElevenLabs por marca de tiempo
    std::sort(mp3_files_to_process_paths.begin(), mp3_files_to_process_paths.end(), [](const auto& a, const auto& b) {
        std::string ts_a = extract_timestamp_from_elevenlabs_filename(a);
        std::string ts_b = extract_timestamp_from_elevenlabs_filename(b);
        return ts_a < ts_b;
    });

    int files_processed_in_this_folder = 0;
    int current_logical_index = 0;

    if (fragments_list_for_this_folder.empty()) { // Modo prefijo normal (esX.mp3 / enX.mp3)
        for (const auto& original_path : mp3_files_to_process_paths) {
            current_logical_index++;
            int actual_number = is_odd_numbered ? (current_logical_index * 2) - 1 : (current_logical_index * 2);
            std::string nuevo_nombre = prefix + std::to_string(actual_number) + ".mp3";
            fs::path final_path = destination_base_folder / nuevo_nombre;

            try {
                fs::copy(original_path, final_path, fs::copy_options::overwrite_existing); // Copiar el original
                std::cout << "✅ Renombrado y copiado a: " << final_path.string() << "\n";
                files_processed_in_this_folder++;
            } catch (const std::exception& e) {
                std::cerr << "❌ Error al copiar/renombrar archivo: " << e.what() << "\n";
            }
        }
    } else { // Modo fragmentos (enXfrY.mp3)
        int original_paths_index = 0; // Índice para recorrer mp3_files_to_process_paths
        for (int f = 0; f < fragments_list_for_this_folder.size(); ++f) {
            int current_fragment_list_idx = f + 1;
            int frase_num = is_odd_numbered ? (current_fragment_list_idx * 2) - 1 : (current_fragment_list_idx * 2);
            int num_frags_in_phrase = fragments_list_for_this_folder[f];

            fs::path subfrases_target_dir = destination_base_folder / ("SubFrases_Frase" + std::to_string(frase_num));
            prepare_target_directory(subfrases_target_dir);

            for (int fr_idx = 1; fr_idx <= num_frags_in_phrase; ++fr_idx) {
                if (original_paths_index >= mp3_files_to_process_paths.size()) {
                    std::cerr << "❗ No hay suficientes archivos para cubrir todos los fragmentos. Procesamiento detenido.\n";
                    total_renamed_files_counter += files_processed_in_this_folder; // Asegurar que se sumen los ya procesados
                    total_renamed_files_counter += files_already_renamed_count;
                    return files_processed_in_this_folder + files_already_renamed_count;
                }

                fs::path original_path = mp3_files_to_process_paths[original_paths_index++];
                std::string nuevo_nombre = prefix + std::to_string(frase_num) + "fr" + std::to_string(fr_idx) + ".mp3";
                fs::path final_path = subfrases_target_dir / nuevo_nombre;

                try {
                    fs::copy(original_path, final_path, fs::copy_options::overwrite_existing);
                    std::cout << "✅ " << nuevo_nombre << " -> guardado en: " << subfrases_target_dir.filename().string() << "\n";
                    files_processed_in_this_folder++;
                } catch (const std::exception& e) {
                    std::cerr << "❌ Error al copiar/renombrar archivo de fragmento: " << e.what() << "\n";
                }
            }
        }
    }

    total_renamed_files_counter += files_processed_in_this_folder;
    total_renamed_files_counter += files_already_renamed_count;
    return files_processed_in_this_folder + files_already_renamed_count;
}


int main() {
    std::cout << "⚠️ Asegurate de que NINGUN archivo este abierto antes de continuar.\n";
    std::cout << "Estas listo para comenzar el renombrado y organizacion automatica? (s/n): ";
    char confirmacion;
    std::cin >> confirmacion;
    if (confirmacion != 's' && confirmacion != 'S') {
        std::cout << "🚫 Cancelado por el usuario.\n";
        return 0;
    }
    std::cin.ignore();

    std::string entrada_fragmentos_impares_str;
    std::cout << "\nIngrese lista de fragmentos para archivos IMPARES (ejemplo: 2,3,1,2): ";
    std::getline(std::cin, entrada_fragmentos_impares_str);
    std::vector<int> fragmentos_impares = dividir_por_coma(entrada_fragmentos_impares_str);

    std::string entrada_fragmentos_pares_str;
    std::cout << "Ingrese lista de fragmentos para archivos PARES (ejemplo: 2,3,1,2): ";
    std::getline(std::cin, entrada_fragmentos_pares_str);
    std::vector<int> fragmentos_pares = dividir_por_coma(entrada_fragmentos_pares_str);

    // Preparar directorios de destino principales
    fs::path frases_spanish_dir = fs::path(RUTA_DESTINO_AUDIOS) / "Frases_Spanish";
    fs::path frases_english_dir = fs::path(RUTA_DESTINO_AUDIOS) / "Frases_English";
    fs::path subfrases_base_dir = fs::path(RUTA_DESTINO_AUDIOS);

    prepare_target_directory(frases_spanish_dir);
    prepare_target_directory(frases_english_dir);

    int total_processed_files = 0;

    // Procesar carpetas de "impares"
    process_audio_folder(fs::path(RUTA_BASE_AUDIOS) / "1_es", frases_spanish_dir, "es", true, {}, total_processed_files);
    process_audio_folder(fs::path(RUTA_BASE_AUDIOS) / "1_en", frases_english_dir, "en", true, {}, total_processed_files);
    process_audio_folder(fs::path(RUTA_BASE_AUDIOS) / "1_subs", subfrases_base_dir, "en", true, fragmentos_impares, total_processed_files);

    // Procesar carpetas de "pares"
    process_audio_folder(fs::path(RUTA_BASE_AUDIOS) / "2_es", frases_spanish_dir, "es", false, {}, total_processed_files);
    process_audio_folder(fs::path(RUTA_BASE_AUDIOS) / "2_en", frases_english_dir, "en", false, {}, total_processed_files);
    process_audio_folder(fs::path(RUTA_BASE_AUDIOS) / "2_subs", subfrases_base_dir, "en", false, fragmentos_pares, total_processed_files);

    std::cout << "\n🎉 Se procesaron y organizaron " << total_processed_files << " archivos correctamente.\n";
    std::cout << "Presiona cualquier tecla para cerrar esta ventana..." << std::endl;
    system("pause");

    return 0;
}
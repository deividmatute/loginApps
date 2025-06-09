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
#include <cstdio> // For _popen, _pclose

using namespace std;
namespace fs = std::filesystem;

// Estructura para almacenar los datos leídos de IndicesImagenes.txt
struct IndicesData {
    std::vector<int> english_only_images;
    std::vector<int> english_spanish_images;
    int total_phrases = 0;
    int total_generated_images = 0;
};

// Función para leer y parsear el archivo IndicesImagenes.txt
IndicesData read_indices_file(const std::string& filename) {
    IndicesData data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
        // En lugar de salir, podemos lanzar una excepción o retornar una estructura vacía/inválida
        // Para este caso, mantenemos exit(EXIT_FAILURE) para una salida rápida en caso de error crítico.
        exit(EXIT_FAILURE); 
    }

    std::string line;
    int line_num = 0;

    // Lee línea por línea y parsea los datos esperados
    while (std::getline(file, line)) {
        line_num++;
        std::stringstream ss(line);
        std::string segment;

        if (line_num == 1) { // Primera línea: índices de imágenes solo en inglés
            while (std::getline(ss, segment, ',')) {
                try {
                    data.english_only_images.push_back(std::stoi(segment));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Formato invalido en la linea 1 de " << filename << ". Esperaba numeros. " << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        } else if (line_num == 2) { // Segunda línea: índices de imágenes en inglés y español
            while (std::getline(ss, segment, ',')) {
                try {
                    data.english_spanish_images.push_back(std::stoi(segment));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Formato invalido en la linea 2 de " << filename << ". Esperaba numeros. " << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        } else if (line_num == 3) { // Tercera línea: total de frases procesadas
            try {
                data.total_phrases = std::stoi(line);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Formato invalido en la linea 3 de " << filename << ". Esperaba un numero entero. " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (line_num == 4) { // Cuarta línea: total de imágenes generadas
            try {
                data.total_generated_images = std::stoi(line);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Formato invalido en la linea 4 de " << filename << ". Esperaba un numero entero. " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    // Verifica que el archivo tenga el número de líneas esperado
    if (line_num < 4) {
        std::cerr << "Error: El archivo " << filename << " no tiene el formato esperado (menos de 4 lineas)." << std::endl;
        exit(EXIT_FAILURE);
    }

    return data;
}

// Clase utilitaria para gestionar archivos temporales. Asegura que se eliminen al salir del alcance.
class TempFile {
    std::string filename;
public:
    explicit TempFile(const std::string& name) : filename(name) {}
    ~TempFile() {
        try { if (!filename.empty()) fs::remove(filename); } catch (...) {}
    }
    std::string path() const { return filename; }
};

// Obtiene el nombre del archivo de una ruta completa
std::string get_filename_from_path_string(const std::string& full_path) {
    size_t last_slash = full_path.find_last_of("/\\"); // Busca la última barra (Windows o Unix)
    if (last_slash == std::string::npos) {
        return full_path; // Si no hay barra, la ruta es el nombre del archivo
    }
    return full_path.substr(last_slash + 1); // Retorna la subcadena después de la última barra
}

// Lista archivos en un directorio que coinciden con un patrón regex y los ordena por el número incrustado en el nombre.
std::vector<std::string> list_files_sorted(const std::string& path, const std::string& pattern) {
    std::vector<std::string> files;
    std::regex re(pattern, std::regex_constants::icase); // Expresión regular para el patrón

    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "Error: El directorio " << path << " no existe o no es un directorio." << std::endl;
            exit(EXIT_FAILURE);
        }
        for (const auto& entry : fs::directory_iterator(path)) { // Itera sobre las entradas del directorio
            std::string filename_str = entry.path().filename().string();
            if (std::regex_match(filename_str, re)) { // Compara el nombre del archivo con el patrón
                files.push_back(entry.path().string());
            }
        }
        // Ordena los archivos extrayendo el número del nombre
        std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
            auto extract_number_from_filename = [](const std::string& s_full_path) -> int {
                std::string filename_part = get_filename_from_path_string(s_full_path);
                std::smatch match;
                // Busca una secuencia de dígitos en el nombre del archivo
                if (std::regex_search(filename_part, match, std::regex(R"(\d+)")) ) {
                    return std::stoi(match[0].str()); // Convierte la secuencia de dígitos a entero
                }
                return 0; // Retorna 0 si no encuentra número (podría indicar un error o un caso base)
            };
            return extract_number_from_filename(a) < extract_number_from_filename(b);
        });
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al leer el directorio: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    // Normaliza las barras de ruta para consistencia (Windows usa '\', Unix usa '/')
    for (auto& file : files) {
        std::replace(file.begin(), file.end(), '\\', '/');
    }
    return files;
}

// Ejecuta un comando del sistema y verifica el código de salida
void exec_command(const std::string& cmd) {
    std::cout << "Ejecutando: " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Error al ejecutar el comando. Codigo de salida: " << ret << std::endl;
        exit(EXIT_FAILURE); // Sale si el comando falla
    }
}

// Crea un archivo de audio de silencio de una duración específica usando FFmpeg
void create_silence_file(const std::string& silence_file, float duration) {
    std::cout << "Creando archivo de silencio: " << silence_file << std::endl;
    // Comando FFmpeg para generar silencio
    std::string cmd = "ffmpeg -y -f lavfi -i anullsrc=r=44100:cl=mono -t " + std::to_string(duration) + " \"" + silence_file + "\"";
    exec_command(cmd);
}

// Obtiene la duración de un archivo de audio usando FFprobe
float get_audio_duration(const std::string& audio_file) {
    std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + audio_file + "\"";
    
    // Ejecuta ffprobe y captura su salida
    FILE* pipe = _popen((cmd + " 2>NUL").c_str(), "r"); // 2>NUL para suprimir salida de error a la consola
    if (!pipe) {
        std::cerr << "Error al obtener la duracion del audio: No se pudo ejecutar ffprobe" << std::endl;
        exit(EXIT_FAILURE);
    }
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer; // Lee la salida de ffprobe
    }
    _pclose(pipe); // Cierra el pipe

    try {
        return std::stof(result); // Convierte la duración (cadena) a float
    } catch (...) {
        std::cerr << "Error al convertir la duracion del audio: " << result << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Concatena un audio con un archivo de silencio usando FFmpeg
void create_audio_with_silence(const std::string& audio, const std::string& silence, const std::string& output) {
    // Comando FFmpeg para concatenar audio y silencio
    std::string cmd = "ffmpeg -y -i \"" + audio + "\" -i \"" + silence + "\" -filter_complex \"[0][1]concat=n=2:v=0:a=1[out]\" -map \"[out]\" \"" + output + "\"";
    exec_command(cmd);
}

// Prepara (limpia y crea) un directorio objetivo
void prepare_target_directory(const fs::path& target_dir_path) {
    if (fs::exists(target_dir_path)) {
        std::cout << "Limpiando directorio existente: " << target_dir_path.string() << "\n";
        try {
            fs::remove_all(target_dir_path); // Elimina todo el contenido recursivamente
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error al limpiar el directorio " << target_dir_path.string() << ": " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }
    }
    try {
        fs::create_directories(target_dir_path); // Crea el directorio (y sus padres si es necesario)
        std::cout << "Directorio preparado: " << target_dir_path.string() << "\n";
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al crear el directorio " << target_dir_path.string() << ": " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}

// Función principal para generar los videos finales a partir de listas de audios e imágenes
void generate_final_video_from_lists(
    float silence_duration_val,
    const std::string& video_name_val,
    const std::vector<std::string>& audios_to_process_final,
    const std::vector<std::string>& images_to_process_final,
    const fs::path& base_output_video_dir, // Nuevo argumento para la ruta base de salida de videos
    const std::string& audio_preparation_output_dir_optional = ""
) {
    // La carpeta de salida de audios temporal estará dentro de la carpeta Librerias (directorio actual)
    const std::string output_audio_dir = "Audios_Generados_Temporales"; 
    
    // La carpeta de salida final del video se construye con la base_output_video_dir
    const fs::path final_video_output_path_for_project = base_output_video_dir;
    
    const std::string final_output_audio_path = output_audio_dir + "/" + video_name_val.substr(0, video_name_val.find_last_of('.')) + "_audio.mp3";
    const std::string final_output_video_path = (final_video_output_path_for_project / video_name_val).string(); // Ruta completa del video final
    const std::string silence_file_temp = "silence_temp_for_concat.mp3";

    // Asegura que los directorios de salida existan
    fs::create_directories(output_audio_dir);
    fs::create_directories(final_video_output_path_for_project); // Asegura que la carpeta del proyecto exista

    // Limpia y crea el archivo de silencio temporal
    if (fs::exists(silence_file_temp)) fs::remove(silence_file_temp);
    create_silence_file(silence_file_temp, silence_duration_val);

    // Concatena cada audio con el silencio temporal
    std::vector<std::string> bloques_audio_final_concat;
    for (size_t i = 0; i < audios_to_process_final.size(); ++i) {
        std::string bloque = output_audio_dir + "/temp_audio_final_" + std::to_string(i) + ".mp3"; // Guardar en Audios_Generados_Temporales
        create_audio_with_silence(audios_to_process_final[i], silence_file_temp, bloque);
        bloques_audio_final_concat.push_back(bloque);
    }

    std::cout << "\nConcatenando audios para el video final (" << video_name_val << ")..." << std::endl;
    TempFile list_audio_final("audio_list_final.txt"); // Archivo temporal para la lista de audios
    {
        std::ofstream out(list_audio_final.path());
        const std::string silence_inicio = output_audio_dir + "/inicio_silencio.mp3"; // Silencio de inicio en el directorio temporal
        if (fs::exists(silence_inicio)) fs::remove(silence_inicio);
        create_silence_file(silence_inicio, 0.7f);
        out << "file '" << silence_inicio << "'\n";
        
        for (const auto& bloque : bloques_audio_final_concat) {
            out << "file '" << bloque << "'\n";
        }
    }
    exec_command("ffmpeg -y -f concat -safe 0 -i " + list_audio_final.path() + " -c copy \"" + final_output_audio_path + "\"");

    std::cout << "\nPreparando lista de imagenes para el video (" << video_name_val << ")..." << std::endl;
    TempFile list_images_final("images_list_final.txt"); // Archivo temporal para la lista de imágenes
    {
        std::ofstream img_out(list_images_final.path());
        for (size_t i = 0; i < bloques_audio_final_concat.size(); ++i) {
            float duration = get_audio_duration(bloques_audio_final_concat[i]);
            if (!fs::exists(images_to_process_final[i])) {
                std::cerr << "Error: La imagen " << images_to_process_final[i] << " no existe. Asegurese de que las imagenes esten generadas y en la ruta correcta." << std::endl;
                exit(EXIT_FAILURE); // Sale si una imagen no se encuentra
            }
            img_out << "file '" << images_to_process_final[i] << "'\n";
            img_out << "duration " << duration << "\n";
        }
        // Añade la última imagen para asegurar que el video no se corte si el último audio es muy corto
        if (!images_to_process_final.empty()) {
            img_out << "file '" << images_to_process_final.back() << "'\n";
        }
    }

    std::cout << "\nGenerando video final: " << video_name_val << "..." << std::endl;
    std::string final_cmd = "ffmpeg -y -f concat -safe 0 -i " + list_images_final.path() +
                             " -i \"" + final_output_audio_path +
                             "\" -map 0:v:0 -map 1:a:0 -c:v libx264 -preset fast -crf 22 -pix_fmt yuv420p -c:a aac -shortest \"" + final_output_video_path + "\"";

    exec_command(final_cmd);
    
    std::cout << "\nEliminando archivos temporales de audio y listas para " << video_name_val << "..." << std::endl;
    int temp_deleted = 0;
    // Elimina los archivos de audio temporales generados
    if (fs::exists(output_audio_dir)) {
        try {
            fs::remove_all(output_audio_dir);
            std::cout << "Directorio temporal de audios eliminado: " << output_audio_dir << "\n";
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error al eliminar el directorio temporal de audios " << output_audio_dir << ": " << e.what() << "\n";
        }
    }
    
    // Elimina el archivo de silencio temporal principal
    if (fs::exists(silence_file_temp)) {
        fs::remove(silence_file_temp);
        ++temp_deleted;
    }
    
    // Elimina el directorio opcional de audios preparados (si se usa y existe)
    if (!audio_preparation_output_dir_optional.empty() && fs::exists(audio_preparation_output_dir_optional)) {
        try {
            fs::remove_all(audio_preparation_output_dir_optional);
            std::cout << "Directorio de audios preparados eliminado: " << audio_preparation_output_dir_optional << "\n";
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error al eliminar el directorio de audios preparados " << audio_preparation_output_dir_optional << ": " << e.what() << "\n";
        }
    }

    std::cout << "\n✅ Video " << video_name_val << " generado exitosamente: " << final_output_video_path << std::endl;
}


int main(int argc, char* argv[]) {
    // El programa ahora espera el nombre de la carpeta del proyecto como argumento
    if (argc < 2) {
        std::cerr << "Error: Se requiere el nombre de la carpeta del proyecto de video como argumento.\n";
        std::cerr << "Uso: " << argv[0] << " <nombre_carpeta_proyecto_video>\n";
        std::cerr << "Ejemplo: " << argv[0] << " Vid0001\n";
        return EXIT_FAILURE;
    }

    std::string video_project_folder_name = argv[1]; // Captura el nombre de la carpeta del proyecto
    std::cout << "Iniciando generacion de videos para el proyecto: '" << video_project_folder_name << "'\n";

    // Define la ruta base para los videos generados específicos de este proyecto
    // Asumimos que este programa (generar_videos.exe) se ejecuta desde MiApp/Librerias/
    // y los videos finales deben ir a MiApp/Videos_Generados/<nombre_proyecto>/
    const fs::path main_videos_output_base_dir = "../Videos_Generados"; // Carpeta Videos_Generados en el nivel de MiApp
    const fs::path current_project_video_output_dir = main_videos_output_base_dir / video_project_folder_name;

    // Asegura que la carpeta de salida del proyecto exista y esté limpia
    prepare_target_directory(current_project_video_output_dir);
    
    // Ejecutar el preprocesador de imágenes (image_preprocessor.exe)
    // Este se ejecuta desde MiApp/Librerias/ y asume que está en el mismo nivel
    std::cout << "Ejecutando el preprocesador de imagenes (image_preprocessor.exe) para preparar los fondos y las imagenes de subtitulos..." << std::endl;
    // Puesto que image_preprocessor.exe limpia su propia carpeta de salida (imagenes_generadas), no necesitamos limpiar antes.
    int preprocessor_ret = std::system("image_preprocessor.exe");
    if (preprocessor_ret != 0) {
        std::cerr << "Error: El preprocesador de imagenes (image_preprocessor.exe) no pudo ejecutarse correctamente o salio con un error. Por favor, asegurese de que este compilado y accesible, y que la fuente 'Montserrat-Bold.ttf' y las imagenes base ('personajes/1000.png', 'personajes/2000.png') esten en sus ubicaciones correctas." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "✅ Preprocesamiento de imagenes completado." << std::endl;


    // Lee los índices de las imágenes generadas por image_preprocessor.exe
    IndicesData indices = read_indices_file("IndicesImagenes.txt");

    float silence_duration;
    string video_name;
    vector<string> audios_to_process;
    vector<string> images_to_process;

    // Rutas base para audios generados
    // Estas rutas están relativas a Librerias/ (donde se ejecuta este programa)
    const string dialogue_audio_base_path = "Audios/Frases_English";
    auto all_dialogue_audios = list_files_sorted(dialogue_audio_base_path, R"(en\d+\.mp3)");
    
    const string personajes_path = "personajes"; // Carpeta personajes en Librerias/
    const string english_images_path = "imagenes_generadas/Imagenes_English"; // Subcarpeta dentro de imagenes_generadas
    const string spanish_images_path = "imagenes_generadas/Imagenes_Spanish"; // Subcarpeta dentro de imagenes_generadas
    const string audio_preparation_output_dir = "Audios_Main_Lesson_Prepared"; // Directorio temporal para audios de Main Lesson
    const string silence_for_preparation_file = "silence_prep.mp3"; // Archivo de silencio temporal

    // --- 1. Generar "Fondo Sin Subtitulos" ---
    std::cout << "\n--- Generando Fondo Sin Subtitulos.mp4 ---\n";
    silence_duration = 1.0f;
    video_name = "Fondo_Sin_Subtitulos.mp4";
    images_to_process.clear();
    audios_to_process.clear();

    for (int i = 0; i < indices.total_phrases; ++i) {
        if (i % 2 == 0) { 
            images_to_process.push_back(personajes_path + "/1000_Listening.png");
        } else {
            images_to_process.push_back(personajes_path + "/2000_Listening.png");
        }
    }
    
    // Asegura que el número de imágenes coincida con el número de audios de diálogo disponibles
    if (images_to_process.size() > all_dialogue_audios.size()) {
        std::cerr << "Advertencia: No hay suficientes archivos de audio de dialogo para todas las imagenes de Fondo Sin Subtitulos. Se usaran los audios disponibles." << std::endl;
        images_to_process.resize(all_dialogue_audios.size()); 
    }
    for(size_t i = 0; i < images_to_process.size(); ++i) {
        audios_to_process.push_back(all_dialogue_audios[i]);
    }
    if (audios_to_process.empty() || images_to_process.empty()) { 
        std::cerr << "Error: No hay audios o imagenes para la opcion Fondo Sin Subtitulos. No se generara este video." << std::endl;
    } else {
        generate_final_video_from_lists(silence_duration, video_name, audios_to_process, images_to_process, current_project_video_output_dir);
    }

    // --- 2. Generar "Fondo con Test" ---
    std::cout << "\n--- Generando Fondo con Test.mp4 ---\n";
    silence_duration = 1.0f;
    video_name = "Fondo_con_Test.mp4";
    images_to_process.clear();
    audios_to_process.clear();

    for (int i = 0; i < indices.total_phrases; ++i) {
        if (i % 2 == 0) { 
            images_to_process.push_back(personajes_path + "/1000_Test.png");
        } else {
            images_to_process.push_back(personajes_path + "/2000_Test.png");
        }
    }
    
    if (images_to_process.size() > all_dialogue_audios.size()) {
        std::cerr << "Advertencia: No hay suficientes archivos de audio de dialogo para todas las imagenes de Fondo con Test. Se usaran los audios disponibles." << std::endl;
        images_to_process.resize(all_dialogue_audios.size()); 
    }
    for(size_t i = 0; i < images_to_process.size(); ++i) {
        audios_to_process.push_back(all_dialogue_audios[i]);
    }
    if (audios_to_process.empty() || images_to_process.empty()) { 
        std::cerr << "Error: No hay audios o imagenes para la opcion Fondo con Test. No se generara este video." << std::endl;
    } else {
        generate_final_video_from_lists(silence_duration, video_name, audios_to_process, images_to_process, current_project_video_output_dir);
    }

    // --- 3. Generar "Fondo con subtitulos en ingles" ---
    std::cout << "\n--- Generando Fondo_Subtitulos_English.mp4 ---\n";
    silence_duration = 1.0f;
    video_name = "Fondo_Subtitulos_English.mp4";
    images_to_process.clear();
    audios_to_process.clear();

    for (int img_idx : indices.english_only_images) {
        images_to_process.push_back("imagenes_generadas/" + std::to_string(img_idx) + ".png"); // Las imágenes están en imagenes_generadas
    }
    
    if (images_to_process.size() > all_dialogue_audios.size()) {
        std::cerr << "Advertencia: No hay suficientes archivos de audio de dialogo para todas las imagenes de Fondo con subtitulos en ingles. Se usaran los audios disponibles." << std::endl;
        images_to_process.resize(all_dialogue_audios.size()); 
    }
    for(size_t i = 0; i < images_to_process.size(); ++i) {
        audios_to_process.push_back(all_dialogue_audios[i]);
    }
    if (audios_to_process.empty() || images_to_process.empty()) { 
        std::cerr << "Error: No hay audios o imagenes para la opcion Fondo con subtitulos en ingles. No se generara este video." << std::endl;
    } else {
        generate_final_video_from_lists(silence_duration, video_name, audios_to_process, images_to_process, current_project_video_output_dir);
    }

    // --- 4. Generar "Fondo con subtitulos en ingles y espanol" ---
    std::cout << "\n--- Generando Fondo_Subtitulos_English_Spanish.mp4 ---\n";
    silence_duration = 1.0f;
    video_name = "Fondo_Subtitulos_English_Spanish.mp4";
    images_to_process.clear();
    audios_to_process.clear();

    for (int img_idx : indices.english_spanish_images) {
        images_to_process.push_back("imagenes_generadas/" + std::to_string(img_idx) + ".png"); // Las imágenes están en imagenes_generadas
    }
    
    if (images_to_process.size() > all_dialogue_audios.size()) {
        std::cerr << "Advertencia: No hay suficientes archivos de audio de dialogo para todas las imagenes de Fondo con subtitulos en ingles y espanol. Se usaran los audios disponibles." << std::endl;
        images_to_process.resize(all_dialogue_audios.size()); 
    }
    for(size_t i = 0; i < images_to_process.size(); ++i) {
        audios_to_process.push_back(all_dialogue_audios[i]);
    }
    if (audios_to_process.empty() || images_to_process.empty()) { 
        std::cerr << "Error: No hay audios o imagenes para la opcion Fondo con subtitulos en ingles y espanol. No se generara este video." << std::endl;
    } else {
        generate_final_video_from_lists(silence_duration, video_name, audios_to_process, images_to_process, current_project_video_output_dir);
    }

    // --- 5. Generar "Main_Lesson.mp4" ---
    std::cout << "\n--- Generando Main_Lesson.mp4 ---\n";
    silence_duration = 2.0f;
    video_name = "Main_Lesson.mp4";
    images_to_process.clear();
    audios_to_process.clear();

    std::cout << "\nPreparando audios para Main Lesson...\n";
    prepare_target_directory(audio_preparation_output_dir); // Directorio temporal para audios preparados
    create_silence_file(silence_for_preparation_file, 0.5f); // Silencio para preparación de audios

    // Rutas base para audios de conversación (relativas a Librerias/)
    const std::string base_audios_conv = "Audios/";
    const std::string en_dir = base_audios_conv + "Frases_English";
    const std::string es_dir = base_audios_conv + "Frases_Spanish";
    const std::string sub_dir_base = base_audios_conv + "SubFrases_Frase"; 

    auto frases_english_audios_source = list_files_sorted(en_dir, R"(en\d+\.mp3)");
    int contador_salida_audios_prepared = 1;

    if (frases_english_audios_source.empty()) {
        std::cerr << "Error: No se encontraron audios base en '" << en_dir << "' para preparar 'Main Lesson'. No se generara este video." << std::endl;
    } else {
        for (const auto& frase_full_path : frases_english_audios_source) { 
            std::string filename_part = get_filename_from_path_string(frase_full_path);
            std::smatch match;
            int num = 0;
            if (std::regex_search(filename_part, match, std::regex(R"(\d+)"))) {
                num = std::stoi(match[0].str());
            }

            std::string en_audio_path = en_dir + "/en" + std::to_string(num) + ".mp3";
            std::string es_audio_path = es_dir + "/es" + std::to_string(num) + ".mp3";
            std::string current_sub_frase_dir = sub_dir_base + std::to_string(num);

            fs::copy(en_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
            fs::copy(en_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);

            if (fs::exists(es_audio_path)) {
                fs::copy(es_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
            } else {
                std::cerr << "Advertencia: Audio en espanol para 'en" << num << ".mp3' no encontrado en '" << es_dir << "'. Usando la frase en ingles como respaldo.\n";
                fs::copy(en_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
            }

            if (fs::exists(current_sub_frase_dir) && fs::is_directory(current_sub_frase_dir)) {
                auto subfrases_audios_source = list_files_sorted(current_sub_frase_dir, R"(en\d+fr\d+\.mp3)"); 
                for (const auto& sub_audio_path : subfrases_audios_source) {
                    fs::copy(sub_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
                    fs::copy(sub_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
                }
            } else {
                std::cout << "Info: Directorio de subfrases '" << current_sub_frase_dir << "' no encontrado o no es un directorio. No se agregaran subfrases.\n";
            }

            fs::copy(en_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
            fs::copy(en_audio_path, audio_preparation_output_dir + "/en" + std::to_string(contador_salida_audios_prepared++) + ".mp3", fs::copy_options::overwrite_existing);
        }
        std::cout << "✅ Audios preparados en: " << audio_preparation_output_dir << std::endl;
        std::cout << "Total de archivos de audio preparados: " << contador_salida_audios_prepared - 1 << std::endl;

        audios_to_process = list_files_sorted(audio_preparation_output_dir, R"(en\d+\.mp3)");
        
        // Las imágenes para Main Lesson se obtienen de imagenes_generadas/
        images_to_process.clear(); // Limpiar antes de llenar
        for (int i = 1; i <= indices.total_generated_images; ++i) {
            images_to_process.push_back("imagenes_generadas/" + std::to_string(i) + ".png");
        }
        
        // Verifica si hay suficientes imágenes para los audios preparados
        if (images_to_process.size() < audios_to_process.size()) {
            std::cerr << "Error: Para 'Main Lesson', se prepararon " << audios_to_process.size()
                      << " audios, pero 'IndicesImagenes.txt' indica solo " << indices.total_generated_images 
                      << " imagenes generadas. Asegurese de tener suficientes imagenes en 'imagenes_generadas/' "
                      << "y que 'total_generated_images' en 'IndicesImagenes.txt' sea al menos " << audios_to_process.size() << "." << std::endl;
            // No se sale, se intentara generar con lo que hay.
        } else if (images_to_process.size() > audios_to_process.size()) {
             std::cerr << "Advertencia: Hay mas imagenes que audios para 'Main Lesson'. Algunas imagenes no se usaran. (Audios: " << audios_to_process.size() << ", Imagenes: " << images_to_process.size() << ")" << std::endl;
             images_to_process.resize(audios_to_process.size()); // Ajustar para que coincidan si hay más imágenes
        }

        if (audios_to_process.empty() || images_to_process.empty()) { 
            std::cerr << "Error: No hay audios o imagenes para la opcion Main Lesson. No se generara este video." << std::endl;
        } else {
            generate_final_video_from_lists(silence_duration, video_name, audios_to_process, images_to_process, current_project_video_output_dir, audio_preparation_output_dir);
        }
    }

    std::cout << "\nFinalizado el procesamiento de videos para el proyecto: '" << video_project_folder_name << "'." << std::endl;
    // No esperar tecla aquí, ya que será llamado por otro exe
    // system("pause"); 

    return EXIT_SUCCESS;
}

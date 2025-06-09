#include <iostream>  // Para entrada/salida de consola
#include <string>    // Para manipulación de cadenas
#include <vector>    // Para vectores dinámicos
#include <fstream>   // Para operaciones de archivo (ofstream y ifstream)
#include <sstream>   // Para stringstream (útil para la conversión de cadena a int)
#include <algorithm> // Para algoritmos como std::sort (no estrictamente necesario para este problema, pero bueno tenerlo)
#include <cctype>    // Para isdigit (para limpiar espacios y validar)
#include <limits>    // Para std::numeric_limits (usado para limpiar el buffer de entrada)

// Define el nombre del archivo de salida para los fragmentos
const std::string FRAGMENTS_OUTPUT_FILE = "cantidadFragmentos.txt";
// Define el nombre del archivo de entrada para los sub-fragmentos
const std::string INPUT_SUB_FRASES_FILE = "Cantidad_Sub_Frases.txt";

int main() {
    std::cout << "Este programa leera los fragmentos del dialogo del archivo '" << INPUT_SUB_FRASES_FILE << "'\n"
              << "y generara el archivo '" << FRAGMENTS_OUTPUT_FILE << "' "
              << "con los digitos de las lineas impares en la primera linea y los de las lineas pares en la segunda.\n";
    std::cout << "------------------------------------------------------------------\n";

    // Abre el archivo de entrada
    std::ifstream input_file(INPUT_SUB_FRASES_FILE);
    if (!input_file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de entrada '" << INPUT_SUB_FRASES_FILE << "'. Asegurese de que exista y sea accesible.\n";
        return EXIT_FAILURE; // Sale del programa con un código de error
    }

    // Vectores para almacenar los números de las posiciones impares (1ra, 3ra, etc.)
    std::vector<int> odd_position_numbers;
    // Vectores para almacenar los números de las posiciones pares (2da, 4ta, etc.)
    std::vector<int> even_position_numbers;

    std::string line;
    int line_num = 0; // Índice de línea basado en 0 para alternar

    // Lee el archivo de entrada línea por línea
    while (std::getline(input_file, line)) {
        // Elimina espacios en blanco al principio y al final de la línea
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        // Si la línea está vacía después de limpiar, la salta
        if (line.empty()) {
            std::cerr << "Advertencia: Linea vacia encontrada en el archivo de entrada. Ignorando.\n";
            continue;
        }

        try {
            // Convierte la línea a un número entero
            int digit = std::stoi(line);
            // Clasifica el dígito según el índice de la línea
            // Las líneas con índice par (0, 2, 4...) corresponden a las posiciones lógicas impares (1ra, 3ra, 5ta)
            if (line_num % 2 == 0) { 
                odd_position_numbers.push_back(digit);
            } else { // Las líneas con índice impar (1, 3, 5...) corresponden a las posiciones lógicas pares (2da, 4ta, 6ta)
                even_position_numbers.push_back(digit);
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: Contenido invalido en la linea " << line_num + 1 << " del archivo '" << INPUT_SUB_FRASES_FILE << "'. Se esperaba un numero entero. Deteniendo procesamiento.\n";
            input_file.close(); // Cierra el archivo antes de salir
            return EXIT_FAILURE;
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: Numero demasiado grande/pequeno en la linea " << line_num + 1 << " del archivo '" << INPUT_SUB_FRASES_FILE << "'. Deteniendo procesamiento.\n";
            input_file.close(); // Cierra el archivo antes de salir
            return EXIT_FAILURE;
        }
        line_num++; // Incrementa el contador de línea para la siguiente iteración
    }
    input_file.close(); // Cierra el archivo de entrada

    // Abre el archivo de salida en modo escritura, sobrescribiendo si ya existe
    std::ofstream output_file(FRAGMENTS_OUTPUT_FILE);

    // Verifica si el archivo de salida se abrió correctamente
    if (!output_file.is_open()) {
        std::cerr << "Error: No se pudo crear/abrir el archivo '" << FRAGMENTS_OUTPUT_FILE << "' para escritura.\n";
        return EXIT_FAILURE;
    }

    // Escribe los números de las posiciones impares en la primera línea del archivo de salida
    for (size_t i = 0; i < odd_position_numbers.size(); ++i) {
        output_file << odd_position_numbers[i];
        if (i < odd_position_numbers.size() - 1) { // Añade una coma si no es el último elemento
            output_file << ",";
        }
    }
    output_file << "\n"; // Nueva línea para separar las listas

    // Escribe los números de las posiciones pares en la segunda línea del archivo de salida
    for (size_t i = 0; i < even_position_numbers.size(); ++i) {
        output_file << even_position_numbers[i];
        if (i < even_position_numbers.size() - 1) { // Añade una coma si no es el último elemento
            output_file << ",";
        }
    }
    output_file << "\n"; // Nueva línea al final del archivo

    output_file.close(); // Cierra el archivo de salida

    // Muestra un resumen de lo generado en la consola
    std::cout << "\n✅ Archivo '" << FRAGMENTS_OUTPUT_FILE << "' generado exitosamente con los fragmentos.\n";
    std::cout << "Contenido generado:\n";
    std::cout << "  Impares (valores de las lineas 1, 3, etc.): ";
    for (size_t i = 0; i < odd_position_numbers.size(); ++i) {
        std::cout << odd_position_numbers[i] << (i < odd_position_numbers.size() - 1 ? "," : "");
    }
    std::cout << "\n";
    std::cout << "  Pares (valores de las lineas 2, 4, etc.):   ";
    for (size_t i = 0; i < even_position_numbers.size(); ++i) {
        std::cout << even_position_numbers[i] << (i < even_position_numbers.size() - 1 ? "," : "");
    }
    std::cout << "\n";

    return EXIT_SUCCESS; // Retorna un código de éxito
}

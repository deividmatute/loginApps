import tkinter as tk
from tkinter import messagebox, simpledialog, Toplevel
import subprocess
import os
import sys
import threading # No necesario si no hay login, pero se mantiene si se agrega funcionalidad de fondo

# Colores (definidos para facilitar la personalizacion)
COLOR_PRIMARY = "#34495e"  # Azul grisaceo oscuro para fondo principal
COLOR_SECONDARY = "#2c3e50" # Azul grisaceo mas oscuro para frames
COLOR_ACCENT = "#3498db"   # Azul vibrante para botones
COLOR_ACCENT_DARK = "#2980b9" # Azul mas oscuro para estado activo
COLOR_TEXT_LIGHT = "white"
COLOR_TEXT_DARK = "#ecf0f1" # Gris claro casi blanco
COLOR_SUCCESS = "#2ecc71"  # Verde para mensajes de exito
COLOR_ERROR = "#e74c3c"    # Rojo para mensajes de error
COLOR_WARNING = "#f39c12"  # Naranja para advertencias

class App:
    def __init__(self, master):
        print("DEBUG: Iniciando App.__init__...") # Debug
        self.master = master
        self.master.title("Lanzador de Aplicaciones")
        self.master.geometry("450x550") # Aumentado el tamano para mejor estetica
        self.master.resizable(False, False)
        self.master.configure(bg=COLOR_PRIMARY) 

        # Centrar la ventana principal
        self.center_window(self.master, 450, 550)

        self.message_label = None # Para mostrar mensajes temporales en la GUI principal
        self.current_process = None # Para mantener un rastro del proceso externo

        # Directamente mostrar el menu principal al inicio
        self.create_main_menu()
        self.master.deiconify() # Asegura que la ventana principal sea visible
        print("DEBUG: Fin de App.__init__.") # Debug

    def center_window(self, window, width, height):
        """Centra una ventana en la pantalla."""
        screen_width = window.winfo_screenwidth()
        screen_height = window.winfo_screenheight()
        x = (screen_width / 2) - (width / 2)
        y = (screen_height / 2) - (height / 2)
        window.geometry(f'{width}x{height}+{int(x)}+{int(y)}')

    def show_custom_message_box(self, message, title="Informacion", color=COLOR_SUCCESS, duration=3000):
        """Muestra una ventana de mensaje personalizada con un estilo mas profesional."""
        msg_box = Toplevel(self.master)
        msg_box.title(title)
        msg_box.config(bg=COLOR_SECONDARY, bd=5, relief="raised")
        self.center_window(msg_box, 350, 150) # Tamano fijo para la caja de mensaje

        # Desactivar boton de cerrar y evitar movimiento
        msg_box.overrideredirect(True) # Quita la barra de titulo y botones de cerrar
        msg_box.grab_set() # Captura todos los eventos del usuario
        
        tk.Label(msg_box, text=message, bg=COLOR_SECONDARY, fg=COLOR_TEXT_LIGHT, 
                 font=("Segoe UI", 12), pady=20, wraplength=300).pack(expand=True)

        # Barra de progreso para visualizacion de duracion
        progress_bar = tk.Canvas(msg_box, height=5, bg=COLOR_PRIMARY, highlightthickness=0)
        progress_bar.pack(fill="x", side="bottom")
        
        # Animacion de la barra de progreso
        self._animate_progress_bar(progress_bar, duration, msg_box)

        msg_box.after(duration, msg_box.destroy) # Cerrar despues de la duracion

    def _animate_progress_bar(self, canvas, duration, msg_box):
        """Anima una barra de progreso."""
        start_time = self.master.winfo_reqwidth()
        step = 10 # Pixeles por paso de animacion
        delay = 50 # Milisegundos entre pasos

        def animate():
            current_width = canvas.winfo_width()
            if current_width > 0: # Solo si la ventana no ha sido destruida
                fill_width = canvas.winfo_width() * (tk.MANDATORY * delay / duration)
                canvas.delete("all")
                canvas.create_rectangle(0, 0, fill_width, canvas.winfo_height(), fill=COLOR_ACCENT, width=0)
                # print(f"DEBUG: Progress: {fill_width}/{canvas.winfo_width()}") # Debug de animacion
                
                # Calcular el tiempo restante y programar el siguiente paso
                elapsed_time = (self.master.winfo_reqwidth() - current_width) * (duration / start_time)
                if elapsed_time < duration:
                    msg_box.after(delay, animate)
            else:
                # La ventana ha sido destruida, parar animacion
                pass

        # Iniciar la animacion
        msg_box.after(delay, animate)


    def show_temp_message_on_main_window(self, message, color="green"):
        """Muestra un mensaje temporal en la interfaz principal (debajo de los botones)."""
        if self.message_label:
            self.message_label.destroy() # Elimina el mensaje anterior

        self.message_label = tk.Label(self.master, text=message, fg=color, bg=COLOR_PRIMARY, font=("Segoe UI", 10, "italic"))
        self.message_label.pack(side="bottom", pady=5)
        self.master.after(5000, self.clear_message_on_main_window) # Borrar mensaje despues de 5 segundos

    def clear_message_on_main_window(self):
        """Borra el mensaje temporal de la ventana principal."""
        if self.message_label:
            self.message_label.destroy()
            self.message_label = None

    def execute_program(self, program_name):
        """Ejecuta un programa externo (.exe) y muestra feedback."""
        if self.current_process and self.current_process.poll() is None:
            messagebox.showwarning("Proceso en Curso", "Ya hay un proceso en ejecucion. Por favor, espere a que termine o cierrelo.")
            return

        try:
            print(f"DEBUG: Lanzando: {program_name}...") # Debug
            self.current_process = subprocess.Popen(program_name, shell=True)
            self.show_custom_message_box(f"Se ha iniciado '{program_name}'.", title="Inicio de Programa", color=COLOR_SUCCESS)
            # Puedes monitorear el proceso si necesitas feedback cuando termina:
            # threading.Thread(target=self._monitor_process, args=(program_name,)).start()
        except FileNotFoundError:
            self.show_custom_message_box(f"Error: No se encontro '{program_name}'. Asegurese de que este en la misma carpeta.", title="Error de Archivo", color=COLOR_ERROR)
        except Exception as e:
            self.show_custom_message_box(f"Error al iniciar '{program_name}': {e}", title="Error de Ejecucion", color=COLOR_ERROR)

    def _monitor_process(self, program_name):
        """Monitorea un proceso en segundo plano (opcional)."""
        if self.current_process:
            self.current_process.wait() # Espera a que el proceso termine
            self.master.after(0, lambda: self.show_custom_message_box(f"'{program_name}' ha terminado su ejecucion.", title="Proceso Finalizado", color="#1abc9c")) # Color turquesa
            self.current_process = None

    def create_main_menu(self):
        """Crea los widgets del menu principal."""
        print("DEBUG: Creando menu principal...") # Debug
        # Limpiar widgets previos si los hubiera (en caso de re-crear el menu)
        for widget in self.master.winfo_children():
            widget.destroy()

        # Estilo de los botones (mas profesional)
        button_style = {
            "bg": COLOR_ACCENT,  # Azul claro
            "fg": COLOR_TEXT_LIGHT,    # Texto blanco
            "font": ("Segoe UI", 12, "bold"), # Fuente mas moderna
            "width": 28, # Un poco mas ancho
            "height": 2,
            "relief": "flat", # Sin borde visible
            "bd": 0,
            "activebackground": COLOR_ACCENT_DARK, # Azul mas oscuro al presionar
            "activeforeground": COLOR_TEXT_LIGHT,
            "cursor": "hand2",
            "highlightbackground": COLOR_ACCENT, # Para border redondeado en algunas OS/versiones
            "highlightthickness": 0,
            "overrelief": "raised" # Efecto sutil al pasar el mouse
        }
        # Para esquinas redondeadas en botones (requiere que el fondo del boton sea igual al del frame)
        # Algunos sistemas operativos/temas pueden ignorar el 'relief' y 'bd' en favor de un estilo nativo.
        # Una forma mas robusta de redondear esquinas seria con un Canvas y un rectangulo o PhotoImage.
        # Pero para simplicidad, Tkinter moderno suele manejarlo bien con bd=0 y flat.


        # Frame para centrar los botones
        frame = tk.Frame(self.master, bg=COLOR_PRIMARY)
        frame.pack(expand=True, pady=20) # Espacio vertical alrededor del frame

        # Titulo en la GUI
        title_label = tk.Label(frame, text="Bienvenido. Elija una opcion:", font=("Segoe UI", 18, "bold"), fg=COLOR_TEXT_LIGHT, bg=COLOR_PRIMARY)
        title_label.pack(pady=25) # Mas espacio para el titulo

        # Botones de las opciones
        btn_renombrar = tk.Button(frame, text="Renombrar Audios", command=self.handle_renombrar_audios, **button_style)
        btn_renombrar.pack(pady=12) # Espacio entre botones

        btn_crear_main = tk.Button(frame, text="Crear Audios Main", command=lambda: self.execute_program("generar_audios_main.exe"), **button_style)
        btn_crear_main.pack(pady=12)

        btn_crear_subtitulos = tk.Button(frame, text="Crear Subtitulos (Imagenes)", command=self.handle_crear_subtitulos, **button_style)
        btn_crear_subtitulos.pack(pady=12)
        
        btn_reparar_volumen = tk.Button(frame, text="Reparar Volumen Audios", command=self.handle_reparar_volumen_audios, **button_style)
        btn_reparar_volumen.pack(pady=12)

        btn_exit = tk.Button(frame, text="Salir", command=self.master.quit, **button_style)
        btn_exit.pack(pady=12)

        # Etiqueta para mensajes temporales (en la ventana principal, en la parte inferior)
        # Se ha movido a show_temp_message_on_main_window
        # self.message_label = tk.Label(self.master, text="", fg=COLOR_SUCCESS, bg=COLOR_PRIMARY, font=("Segoe UI", 10, "italic"))
        # self.message_label.pack(side="bottom", pady=5)


    def handle_renombrar_audios(self):
        """Maneja la logica para "Renombrar Audios"."""
        if not messagebox.askyesno("Confirmacion", "¿Esta listo para comenzar el renombrado y organizacion automatica?"):
            self.show_temp_message_on_main_window("Operacion 'Renombrar Audios' cancelada.", color=COLOR_WARNING)
            return

        odd_fragments_str = simpledialog.askstring("Listas de Fragmentos", 
                                                    "Ingrese lista de fragmentos para archivos IMPARES (ejemplo: 2,3,1,2):",
                                                    initialvalue="2,3,1,2", parent=self.master) # Añadir parent
        if odd_fragments_str is None:
            self.show_temp_message_on_main_window("Entrada de fragmentos IMPARES cancelada.", color=COLOR_WARNING)
            return

        even_fragments_str = simpledialog.askstring("Listas de Fragmentos",
                                                     "Ingrese lista de fragmentos para archivos PARES (ejemplo: 2,3,1,2):",
                                                     initialvalue="2,3,1,2", parent=self.master) # Añadir parent
        if even_fragments_str is None:
            self.show_temp_message_on_main_window("Entrada de fragmentos PARES cancelada.", color=COLOR_WARNING)
            return

        if self.write_fragments_to_file(odd_fragments_str, even_fragments_str):
            self.execute_program("generar_nombres_audios.exe")

    def write_fragments_to_file(self, odd_fragments_str, even_fragments_str, filename="cantidadFragmentos.txt"):
        """Escribe las cadenas de fragmentos en el archivo especificado."""
        try:
            with open(filename, "w") as f:
                f.write(odd_fragments_str.strip() + "\n")
                f.write(even_fragments_str.strip() + "\n")
            print(f"Listas de fragmentos guardadas en '{filename}'")
            return True
        except Exception as e:
            self.show_temp_message_on_main_window(f"Error: No se pudo escribir en '{filename}': {e}", color=COLOR_ERROR)
            return False

    def handle_crear_subtitulos(self):
        """Maneja la logica para "Crear Subtitulos (Imagenes)"."""
        reminder_msg = "Recuerde: Debe tener los nuevos personajes en la carpeta 'personajes' y haber actualizado el archivo 'TXT Excel.txt' antes de continuar."
        if messagebox.askyesno("Advertencia", reminder_msg + "\n\n¿Desea continuar?"):
            self.execute_program("imagenes.exe")
        else:
            self.show_temp_message_on_main_window("Operacion 'Crear Subtitulos' cancelada.", color=COLOR_WARNING)

    def handle_reparar_volumen_audios(self):
        """Maneja la logica para "Reparar Volumen Audios"."""
        warning_msg = "⚠️ ATENCION: Este programa sobrescribira los archivos MP3 de la carpeta 'Audios'. ¡Cree una copia de seguridad antes de continuar!"
        if messagebox.askyesno("Advertencia Importante", warning_msg + "\n\n¿Desea continuar?"):
            self.execute_program("normalizar_audios.exe")
        else:
            self.show_temp_message_on_main_window("Operacion 'Reparar Volumen Audios' cancelada.", color=COLOR_WARNING)


if __name__ == "__main__":
    print("Iniciando script Python de la GUI...") # Debug: Confirmacion de inicio del script
    root = tk.Tk()
    app = App(root)
    root.mainloop()

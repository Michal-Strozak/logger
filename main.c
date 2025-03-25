#include "logger.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main() {
    // Inicjalizacja loggera
    if (init_logger() != 0) {
        fprintf(stderr, "Błąd podczas inicjalizacji loggera\n");
        return 1;
    }
    write_log(1, "Test logów");

    // Zaczynamy testowanie
    printf("Testowanie loggera...\n");


    // Wysłanie sygnału do zapisania zrzutu stanu
    kill(getpid(), SIG_DUMP);
    sleep(1);  // Czekamy chwilę na zapis do dump.log

    // Wysłanie sygnału do włączenia/wyłączenia logowania
    kill(getpid(), SIG_LOG_TOGGLE);
    sleep(1);  // Czekamy na przełączenie logowania

    // Wysłanie sygnału do zmiany poziomu logowania
    kill(getpid(), SIG_LOG_LEVEL);
    sleep(1);  // Czekamy na zmianę poziomu

    // Zamknięcie loggera
    close_logger();

    return 0;
}

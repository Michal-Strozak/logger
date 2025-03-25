# logger
Celem projektu jest napisanie biblioteki dostarczającej systemu logowania do plików stanu aplikacji sterowanego sygnałami. System logowania powinien spełniać następujące założenia:

powinien być sterowany sygnałami czasu rzeczywistego i wykorzystywać przenoszone z nimi dane;
obsługa sygnałów powinna być realizowana przez kod wykonywany asynchronicznie;
jeden z sygnałów sterujących powinien umożliwiać zapis do oddzielnego pliku aktualnego stanu wewnętrznego aplikacji (plik zrzutu - dump), zawartość pliku powinna być zależna od aplikacji i w niej konfigurowalna, każde wystąpienie sygnału powinno generować oddzielny plik;
drugi z sygnałów powinien włączać/wyłączać logowanie zdarzeń do pliku logowania (log);
trzeci z sygnałów powinien umożliwiać przełączenie poziomu szczegółowości logowania (trzy poziomy: MIN, STANDARD, MAX);
funkcja zapisująca komunikat do pliku logowania powinna przyjmować jako argument jego istotność, decyzja o zapisie komunikatu lub jego zaniechaniu powinna być podejmowana na podstawie porównania jego istotności z aktualnym poziomem szczegółowości;
biblioteka musi działać prawidłowo w aplikacjach wielowątkowych;
należy zadbać o prawidłową inicjalizację i zamknięcie biblioteki;
kod źródłowy biblioteki powinien być zgodny ze standardami POSIX i języka C (C++).

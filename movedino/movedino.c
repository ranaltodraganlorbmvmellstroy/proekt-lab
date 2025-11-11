#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_W 100
#define MAX_H 100
#define MAX_HISTORY 10000//макс колво шогов для UNDO

//создаём клетку с характеристиками : цвет и объект на клеткt
    typedef struct{
        char obj;  //будет хранить в себе объект типа дерево итд вяску таку фигню
        char color; //будет хранить цвет
    }cell;
//создаём поле , которое будет хранить и передовать положение клеток и динозавра
    typedef struct 
    {
        int shirina , height;//ширина и высота поля
        cell kletki[MAX_H][MAX_W];//максимальная высота и ширина поля, где каждый элемент типа cell
    }pole;
//создаём самого динозавра
    typedef struct
    {
        int x, y;//координаты динозавра
        int exists; //проверяем существует ли динозавр на поле , чтобы START не выполнялся повторно и др команды MOVE JUMP DIG итд сначала проверяют наличие exists == 1 
    }dino;
//создадим структуру с помощью которой мы будем запоминать на шаги то есть историю ходов UNDO
    typedef struct 
    {
        pole pole_status; //текущая позиция поля (все клетки , цыета и объекты)
        dino dino_status; //текущая позиция динозавра
    }status;
//структура для вычисления координат в тородильном пространстве
    typedef struct 
    {
        int x, y;
    }coord;
//глобальные данные ,присваеваем 0 вместо шума
    pole pole_status = {0};//вместо шума буду инициализированны нули
    dino dino_status = {0}; //то же самое ^
    status history[MAX_HISTORY];  //глобальное хранение всей истории для UNDO (каждый элемент зранит в себе "сникмок" поля)
    int history_size = 0; //счетчик истории для записив массив 
//настройка интерпритатора 
int use_display = 1;//показывает нужно ли выводить текущее состояние поля в консоль после каждой команды
int use_save = 1;//показывает нуно ли сохраянть состояние в выходной файл
int delay_sec = 1;//задержка перед выводом на поле, чтобы было видно перемещение динозавра
//функция для очистки экрана для разных ОС
void clear_screen(){
    if(!use_display) return;//если дисплей не нажно выводить, конец функции
#ifdef _WIN32 //если _WIN32 определён ,то вызывает строку cls (это для винды)
    system("cls");
#else 
    system("clear"); //если не определён , вызывает clear (для Unix систем)
#endif //конец компиляции
}
//функция для сохранения status в историю 
void save_status(){
    if(history_size >= MAX_HISTORY) return; //если будет переполнение массива истоий , то заканчивать функцию
    history[history_size].pole_status = pole_status;
    history[history_size].dino_status = dino_status;
    //ну тут мы в обоих случаях просто записываем данные , когда вызываем функцию
    history_size++;
}
//функция для отката UNDO
void UNDO(){
    //проверяем есть ли состояние для отката
    if(history_size <= 0){
        fprintf(stderr, "UNDO, нет состояний для отката");//выводим ошибку в спец поток для ошибок
        return;
    }
    history_size--;//откатываем
    pole_status = history[history_size].pole_status;
    dino_status = history[history_size].dino_status;//присваимваем откаченные данные
}
//создаём фуецию для координат клетки в тороидальном пространстве
coord toroid_coord(int x, int y){
    coord result;
    result.x = ((x % pole_status.shirina + pole_status.shirina) % pole_status.shirina);//тут мы считаем позицию в тор пространстве , с учетом того ,что число может быть отрицательнм
    result.y = ((y % pole_status.height + pole_status.height) % pole_status.height);//аналогично
    return result;
}
//получаем клетку в торовом пространстве. нужна будет, чтобы другие функции получали сразу свойства нужной клетки тк (на выход идёт переменная cell с координатами Tor x и Tor y)
cell get_cell(int x, int y){
    coord Tor = toroid_coord(x, y);
    return pole_status.kletki[Tor.y][ Tor.x];
}
//создадим функцию, которая будет устанавливать объект ,но сохранять цвет. с помощью этой функции наш динозаврик сможет взаимодействовать с полем 
void set_cell(int x, int y, char obj){
    coord Tor = toroid_coord(x, y);

    char old_color = pole_status.kletki[Tor.y][Tor.x].color;

    if (obj >= 'a' && obj <= 'z') {//так как в таблице ASCII все символы идут по порядку, а буквы можно представить ввиде чисел a == 97 z == 122
        // Это покраска
        // При покраске мы НЕ меняем obj, а только цвет
        pole_status.kletki[Tor.y][Tor.x].color = obj;
        // obj остаётся каким был
    } else {
        // Обычный объект, цвет сохраняется
        pole_status.kletki[Tor.y][Tor.x].obj = obj;
        pole_status.kletki[Tor.y][Tor.x].color = old_color;
    }
/*вообще эта функция,на половину, нужня для предотвращения ошибок (по типу динозавр 
становится невидимым, если он поставит цвет под себя итд)*/
}
//функция , которая преобразует строку направления в смещения
void KudaNadoDvigatsya(const char* dir, int* dx, int* dy) {
    *dx = 0;
    *dy = 0;/*ну тут указатели, чтобы работать именно с той частью памяти,а не создавать новые переменные,
             заполняя стек ну и это удобнее(обнуляем значения по адресам указателя)*/
    if (strcmp(dir, "UP") == 0) { *dy = -1; }//с помощью функции srtcmp сравниваем массивы и если они одинаковые ,то 0 ==0, выполняется
    else if (strcmp(dir, "DOWN") == 0) { *dy = 1; }
    else if (strcmp(dir, "LEFT") == 0) { *dx = -1; }
    else if (strcmp(dir, "RIGHT") == 0) { *dx = 1; }//аналогично с UP
    else {
        fprintf(stderr, "Неизвестное направление: %s\n", dir);//елси ввеллось некоректное направление, то выводим ошибку
        exit(1);
    }
}
//функци, инициализирующая пустое поле 
void init_pole(int shirinaPole, int visotaPole){
    pole_status.shirina = shirinaPole;
    pole_status.height = visotaPole;
    for(int i = 0; i < shirinaPole; i++){
        for(int y = 0; y < visotaPole; y++){
            pole_status.kletki[y][i].obj = '_';
            pole_status.kletki[y][i].color = 0;
        }
    }
/*ну тут функция просто пробегает все клетки заданного поля и инициализирует их нулями и '_'*/
}
//вывод поля
void print_pole(){
    if(!use_display) return;
    clear_screen();  //очищаем консоль
    for(int y = 0; y < pole_status.height; y++){
        for(int x = 0; x < pole_status.shirina; x++){
            char simvol = pole_status.kletki[y][x].obj;
            if(simvol == '_' && pole_status.kletki[y][x].color){//если клетка пустая и у неё есть цвет (те pole_status.kletki[y][x].color != 0),то выводим цвет 
                putchar(pole_status.kletki[y][x].color);
            } else {
            putchar(simvol);//ну если клетка не пустая, то выводим pole_status.kletki[y][x].obj
            }
        }
    putchar('\n');
    fflush(stdout);//сбрасываем буффер ,чтобы все вывелось
    if(delay_sec > 0) sleep(delay_sec);//включаем задержку
    }
}
//функция, помещающая динозавра на поле
void place_dino(int x, int y) {
    if (x < 0 || x >= pole_status.shirina || y < 0 || y >= pole_status.height) { //проверяем, чтобы координаты были в пределах поля
        fprintf(stderr, "Ошибка: START вне поля.\n"); //fprintf выберает поток ,куда будет выводиться ошибка
        exit(1);//завершает выполнение программы
    }
    dino_status.x = x;    //устанавливаем координаты динозавра
    dino_status.y = y;
    dino_status.exists = 1;//отмечаем, что динозавр существует
    set_cell(x, y, '#'); //ставим динозавра на поле
}
//функция, для перемещения динозавра на 1 клетку 
void move_dino(const char* dir) {
    int dx, dy;//перемещение динозавра
    //получаем смещения по направлению
    KudaNadoDvigatsya(dir, &dx, &dy);//определяем куда надо двигать по значению массива dir
    //получаем клетку, в которую хотим пойти
    cell c = get_cell(dino_status.x + dx, dino_status.y + dy);
    //проверяем, не яма ли это
    if (c.obj == '%') {
        fprintf(stderr, "Ошибка: динозавр упал в яму!\n");
        exit(1);//программ заканчивается ,выводится ошибка
    }
    //проверяем, не препятствие ли это
    if (c.obj == '^' || c.obj == '&' || c.obj == '@') {
        fprintf(stderr, "Предупреждение: движение заблокировано.\n");
        return;//ну тут просто выводится предупреждение в поток ошибок
    }
      // Освободить старую позицию динозавра
    char old_color = pole_status.kletki[dino_status.y][dino_status.x].color;//ну тут для читаемости
    pole_status.kletki[dino_status.y][dino_status.x].obj = (old_color ? old_color : '_');//тут если олдоколор не равен 0 , то выводим его же, иначе '_'

    //вычисляем новую позицию с учётом тороидальности пространства
    coord dest = toroid_coord(dino_status.x + dx, dino_status.y + dy);
    dino_status.x = dest.x;
    dino_status.y = dest.y;
    //ставим динозавра в новую позицию
    set_cell(dest.x, dest.y, '#');
/*кратркая сводка по функции : определяем куда двигаться -> получаем данные клетки на которую хотим двигаться -> проверяем на возможнось двигаться (если 
там яма то заканчиваем программу) -> освобождаем старую и заполняем новую позицию(c учётом тороидальной топологии)*/
}
//сделаем функцию для прыжка
void jump_dino(const char* dir, int n) {//
    int dx, dy;
    //получаем смещения
    KudaNadoDvigatsya(dir, &dx, &dy);

    //начинаем с текущей позиции динозавра
    int x = dino_status.x, y = dino_status.y;//чтобы сразу не изменять реальные кординаты
    int hit = 0;
    //цикл по n шагам прыжка
    for (int i = 1; i <= n; i++) {
        cell c = get_cell(x + dx, y + dy);//получаем данные куда хотим переместиться
        //если встречаем препятствие, останавливаемся
        if (c.obj == '^' || c.obj == '&' || c.obj == '@') {
            hit = 1;
            fprintf(stderr, "Прыжок остановлен препятствием.\n");
            break;
        }
        //переходим к следующей позиции
        coord next = toroid_coord(x + dx, y + dy);
        x = next.x;
        y = next.y;
    }  
    /*делаем n шагов в нарпавлении и если встречается препятствие, то отсанавлваемся и выводим ошибку в поток оибок*/

    //проверяем, куда приземлились
    cell c = get_cell(x, y);
    //если приземлились в яму и не наткнулись на препятствие, ошибка
    if (!hit && c.obj == '%') {
        fprintf(stderr, "Ошибка: приземлился в яму!\n");
        exit(1);//завершение программы
    }
/*============================================================================================*/
    //освобождаем старую позицию
    char old_color = pole_status.kletki[dino_status.y][dino_status.x].color;
    pole_status.kletki[dino_status.y][dino_status.x].obj = (old_color ? old_color : '_');

    //вычисляем конечную позицию
    coord landing = toroid_coord(x, y);
    dino_status.x = landing.x;
    dino_status.y = landing.y;
    //ставим динозавра в новую позицию
    set_cell(landing.x, landing.y, '#');
    //ну в этом блоке аналогично предидущему все происходит
/*============================================================================================*/
}
// Получить соседнюю клетку относительно динозавра(возвращает тип cell и данные нужной клетки)
cell get_neighbor_cell(const char* dir) {//константным значение мы показываем, что не будем менять *dir
    int dx, dy;
    //получаем смещения
    KudaNadoDvigatsya(dir, &dx, &dy);
    //возвращаем клетку в направлении
    return get_cell(dino_status.x + dx, dino_status.y + dy);
/*вообзе для чего нужна эта функция : для команд DIG, GROW , PUSH, где нужно посмотреть ,что в сосед. клетке*/
}
void pomestit_obj_na_cell_ronalda(const char *dir, char obj){//ну тут даже говорить нечего
    int dy, dx;
    KudaNadoDvigatsya(dir, &dx, &dy );
    coord objekta = toroid_coord(dino_status.x + dx, dino_status.y + dy);
    set_cell(objekta.x, objekta.y, obj);
}
// Создать объект рядом
void create_obj(const char* dir, char obj) {
    cell c = get_neighbor_cell(dir);
    //проверяем, можно ли создать объект (пустая клетка)
    if (c.obj != '_' && !(c.obj >= 'a' && c.obj <= 'z')) {
        fprintf(stderr, "Ошибка: клетка занята (%c).\n", c.obj);
        exit(1);
    }
    //устанавливаем объект
    pomestit_obj_na_cell_ronalda(dir, obj);
}
// Срубить дерево
void cut_tree(const char* dir) {
    cell c = get_neighbor_cell(dir);//получем данные нужной cell
    //проверяем, есть ли дерево
    if (c.obj != '&') {
        fprintf(stderr, "Ошибка: нет дерева в указанной клетке.\n");
        return;
    }
    // Убираем дерево, цвет остаётся
    int dx, dy;
    KudaNadoDvigatsya(dir, &dx, &dy);

    coord pos = toroid_coord(dino_status.x + dx, dino_status.y + dy);

    char color = pole_status.kletki[pos.y][pos.x].color;
    pole_status.kletki[pos.y][pos.x].obj = (color ? color : '_');
    /*когда срубаем проверяем какой цвет был по деревом ( и был ли вообще) и устанавлеваем его (цвет) */
}
// Пнуть камень
void push_stone(const char* dir) {
    cell c = get_neighbor_cell(dir);
    //проверяем, есть ли камень
    if (c.obj != '@') {
        fprintf(stderr, "Ошибка: нет камня.\n");
        return;
    }

    int dx, dy;
    //получаем смещение
    KudaNadoDvigatsya(dir, &dx, &dy);

    //координаты камня
    coord stone_pos = toroid_coord(dino_status.x + dx, dino_status.y + dy); //находим координаты камня относительно координат динозаврика

    //направление, куда катится камень - в том же направление , что и динозавр пинает
    coord target_pos = toroid_coord(stone_pos.x + dx, stone_pos.y + dy);//находим координаты возможного таргета

    //проверяем, куда катится камень
    cell target = get_cell(stone_pos.x + dx, stone_pos.y + dy);

    //проверяем, не препятствие ли это
    if (target.obj == '&' || target.obj == '^' || target.obj == '@') {
        fprintf(stderr, "Камень упёрся.\n");
        return;
    }

    // Убрать камень
    char stone_color = pole_status.kletki[stone_pos.y][stone_pos.x].color;
    pole_status.kletki[stone_pos.y][stone_pos.x].obj = (stone_color ? stone_color : '_');

    // Поставить в новую
    if (target.obj == '%') {
        // Засыпает яму
        char target_color = pole_status.kletki[target_pos.y][target_pos.x].color;
        pole_status.kletki[target_pos.y][target_pos.x].obj = (target_color ? target_color : '_');
    } else {
        set_cell(target_pos.x, target_pos.y, '@');
    }
/*краткая сводка : программа находит данные соседней клетки ,проверяет есть ли там камне,иначе выдаёт ошибку находим коорд камня
находим смещение (уже для камня), проверяем не препятстви ли , пото стандарт ный алгоритм , но только в конце мы еще засыпаем яму*/
}
//=========================================================РАБОТАЕМ С ФАЙЛОМ=====================================================================//
// Загрузить поле из файла (LOAD)
void load_from_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    /*FILE - узказатель на файл(если его не удалось открыть то NULL), fopen пытается открыть файл filename для чтения "r" - read*/
    if (!f) { perror("LOAD"); exit(1)/*завершение программы*/; }
    /*if (!f) если == NULL , то ошибка perror("LOAD") выводит системное сообщение об ошибке открытия файла */
    int w, h;
    if (fscanf(f, "%d %d", &w, &h) != 2 || w < 10 || w > MAX_W || h < 10 || h > MAX_H) {/*если не прочитал ровно 2 числа то ошибка, проверяет допустимые пределы*/
        fprintf(stderr, "Ошибка: недопустимый формат LOAD.\n");
        fclose(f);//закрывает файл, чтобы не оставлять ресурсы открытыми
        exit(1);
    }
    /*fscanf(f, "%d %d", &w, &h) - читает два первых числа из файла и присваиваеит их w и h*/
    init_pole(w, h);
    dino_status.exists = 0;
    //ну тут понятно
    char line[MAX_W + 2];//создаём буфер для чтения одной строки из filename
    for (int y = 0; y < h; y++) {//цикл для всех строк поля 
        if (!fgets(line, sizeof(line), f)) {// читаем одну строку из файла f в буффер line
            fprintf(stderr, "Ошибка: не хватает строк в LOAD.\n");
            fclose(f);
            exit(1);//если не удалось считать ,то закрываем и выводим ошибку + конец программы
        }
        for (int x = 0; x < w; x++) {
            char c = line[x];
            if (c == '\n') break;
            if (c >= 'a' && c <= 'z') {
                pole_status.kletki[y][x].obj = c;
                pole_status.kletki[y][x].color = c;
            } else {
                pole_status.kletki[y][x].obj = c;
                pole_status.kletki[y][x].color = 0;
                if (c == '#') {
                    dino_status.x = x;
                    dino_status.y = y;
                    dino_status.exists = 1;
                }
            }
        /*пробегаемся по все столбцам и строкам и если строчная буква , то obj и color = c и если не буква , то color = 0 и obj = c, если символ == #, то 
        это динозавр и мы его устанавливаем и делаем его статус : существует(dino_status.exists = 1)*/
        }
    }
    fclose(f);
/*цель этой функции считать файл (f) input.txt или output.txt (если бы результатом прошлого запуска)
и интерпритировать его как состояние игрового поля(собственно интерпретатор) */
}

// Выполнить файл (EXEC) - заглушка, тело будет ниже
void execute_file(const char* filename, int* line_counter);


// Обработка одной строки
void process_line(char* line, int* line_counter, const char* source) {
    /*удаляем символы конца строки
    Находим первое вхождение символа \r или \n в строке line
    strcspn возвращает индекс этого символа (или длину строки, если не найдено)
     Заменяем этот символ на '\0' (нулевой символ), чтобы строка "обрывалась" до него
     нужно для коректной обработки кода , чтобы слова не переносили там где не надо*/
    line[strcspn(line, "\r\n")] = 0;

    //пропускаем комментарии и пустые строки
    // Если строка пустая (первый символ - нулевой символ) или начинается с "//"
    if (line[0] == 0 || (line[0] == '/' && line[1] == '/')) return; // выходим из функции, не обрабатываем дальше

    // Убираем хвостовые пробелы(MOVE UP__\n) по типу такого, где __ - пробел
    // Получаем длину строки
    int len = strlen(line);
    // Пока длина больше 0 и последний символ строки - пробельный (пробел, табуляция и тд)
    while (len > 0 && isspace((unsigned char)line[len - 1])) {//isspasce проверяет является ли символ пробельным
        // Уменьшаем длину и заменяем последний символ на '\0', укорачивая строку
        line[--len] = 0;
    }

    // Объявляем буфер для хранения команды (названия действия, например, "MOVE")
    char cmd[50];
    // Пытаемся извлечь первое слово из строки line и записать в cmd
    // %49s - значит, не более 49 символов + '\0'
    // Если не удалось извлечь слово (sscanf вернул 0), выходим
    if (sscanf(line, "%49s", cmd) != 1) return;//sscanf читает не с клавиатуры , а из строки в нашем случа е line

    //сохраняем состояние перед выполнением команды
    // Это нужно для команды UNDO: мы сохраняем "снимок" текущего состояния поля и динозавра
    save_status();
    /*=============================================большой однотипный блок============================================= */
    // Проверяем, какая команда была извлечена, и выполняем соответствующие действия
    if (strcmp(cmd, "SIZE") == 0) {
        int w, h; // переменные для ширины и высоты
        // Проверяем, что это первая строка (кроме комментариев), формат корректный, и размеры в допустимых пределах
        if (*line_counter != 1 || sscanf(line, "SIZE %d %d", &w, &h) != 2 ||
        /*sscanf ,если успешно прочитал 2 чсила , то вовращает 2 , если одно , то 1 , если ноль , то 0, если выдаёт -1 то ошибка ,
        короче проверяем , чтобы прочитал именно 2 числа , иначе ошибка*/
            w < 10 || w > MAX_W || h < 10 || h > MAX_H) {
            fprintf(stderr, "Ошибка в SIZE (должна быть первой, корректные размеры).\n");
            exit(1); // завершаем программу с ошибкой
        }
        // Инициализируем поле с новыми размерами
        init_pole(w, h);
    }
    // Аналогично проверяем и обрабатываем команду START
    else if (strcmp(cmd, "START") == 0) {
        int x, y; // координаты
        // Проверяем, что SIZE был задан, формат корректный
        if (!pole_status.shirina || sscanf(line, "START %d %d", &x, &y) != 2) {
            fprintf(stderr, "Ошибка: SIZE должен быть до START.\n");
            exit(1);
        }
        // Проверяем, что START не вызывается повторно
        if (dino_status.exists) {
            fprintf(stderr, "Ошибка: повтор START.\n");
            exit(1);
        }
        // Размещаем динозавра на поле
        place_dino(x, y);
    }
    // Обработка команды MOVE
    else if (strcmp(cmd, "MOVE") == 0) {
        // Проверяем, что динозавр существует
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для MOVE.\n"); exit(1); }
        char dir[20]; // буфер для направления
        // Пытаемся извлечь направление
        if (sscanf(line, "MOVE %19s", dir) != 1) { fprintf(stderr, "MOVE: не указано направление.\n"); exit(1); }
        // Вызываем функцию перемещения
        move_dino(dir);
    }
    // Обработка команды JUMP
    else if (strcmp(cmd, "JUMP") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для JUMP.\n"); exit(1); }
        char dir[20];
        int n; // количество клеток для прыжка
        // Извлекаем направление и число
        if (sscanf(line, "JUMP %19s %d", dir, &n) != 2 || n <= 0) {
            fprintf(stderr, "JUMP: направление и положительное число.\n");
            exit(1);
        }
        // Вызываем функцию прыжка
        jump_dino(dir, n);
    }
    // Обработка команды PAINT
    else if (strcmp(cmd, "PAINT") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для PAINT.\n"); exit(1); }
        char c; // символ для покраски
        // Извлекаем символ
        if (sscanf(line, "PAINT %c", &c) != 1 || c < 'a' || c > 'z') {
            fprintf(stderr, "PAINT: строчная латинская буква.\n");
            exit(1);
        }
        // Красим клетку, на которой стоит динозавр
        set_cell(dino_status.x, dino_status.y, c);
    }
    // Обработка команды DIG
    else if (strcmp(cmd, "DIG") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для DIG.\n"); exit(1); }
        char dir[20];
        if (sscanf(line, "DIG %19s", dir) != 1) { fprintf(stderr, "DIG: направление.\n"); exit(1); }
        // Создаём яму в соседней клетке
        create_obj(dir, '%');
    }
    // Обработка команды MOUND
    else if (strcmp(cmd, "MOUND") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для MOUND.\n"); exit(1); }
        char dir[20];
        if (sscanf(line, "MOUND %19s", dir) != 1) { fprintf(stderr, "MOUND: направление.\n"); exit(1); }
        // Создаём гору в соседней клетке
        create_obj(dir, '^');
    }
    // Обработка команды GROW
    else if (strcmp(cmd, "GROW") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для GROW.\n"); exit(1); }
        char dir[20];
        if (sscanf(line, "GROW %19s", dir) != 1) { fprintf(stderr, "GROW: направление.\n"); exit(1); }
        // Создаём дерево в соседней клетке
        create_obj(dir, '&');
    }
    // Обработка команды CUT
    else if (strcmp(cmd, "CUT") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для CUT.\n"); exit(1); }
        char dir[20];
        if (sscanf(line, "CUT %19s", dir) != 1) { fprintf(stderr, "CUT: направление.\n"); exit(1); }
        // Срубаем дерево в соседней клетке
        cut_tree(dir);
    }
    // Обработка команды MAKE
    else if (strcmp(cmd, "MAKE") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для MAKE.\n"); exit(1); }
        char dir[20];
        if (sscanf(line, "MAKE %19s", dir) != 1) { fprintf(stderr, "MAKE: направление.\n"); exit(1); }
        // Создаём камень в соседней клетке
        create_obj(dir, '@');
    }
    // Обработка команды PUSH
    else if (strcmp(cmd, "PUSH") == 0) {
        if (!dino_status.exists) { fprintf(stderr, "Нет динозавра для PUSH.\n"); exit(1); }
        char dir[20];
        if (sscanf(line, "PUSH %19s", dir) != 1) { fprintf(stderr, "PUSH: направление.\n"); exit(1); }
        // Пинаем камень в соседней клетке
        push_stone(dir);
    }
    // Обработка команды LOAD
    else if (strcmp(cmd, "LOAD") == 0) {
        // LOAD должна быть первой командой
        if (*line_counter != 1) {
            fprintf(stderr, "LOAD должна быть первой.\n");
            exit(1);
        }
        char fname[256]; // буфер для имени файла
        if (sscanf(line, "LOAD %255s", fname) != 1) {
            fprintf(stderr, "LOAD: имя файла.\n");
            exit(1);
        }
        // Загружаем поле из файла
        load_from_file(fname);
    }
    // Обработка команды EXEC
    else if (strcmp(cmd, "EXEC") == 0) {
        char fname[256];
        if (sscanf(line, "EXEC %255s", fname) != 1) {
            fprintf(stderr, "EXEC: имя файла.\n");
            exit(1);
        }
        // Выполняем команды из другого файла
        execute_file(fname, line_counter);
        return; // после EXEC текущую строку обрабатывать не нужно (print_pole вызывается внутри execute_file)
    }
    // Обработка команды UNDO
    else if (strcmp(cmd, "UNDO") == 0) {
        // Откатываем состояние поля и динозавра
        UNDO();
    }
    // Обработка условной команды IF
    else if (strcmp(cmd, "IF") == 0) {
        int x, y; // координаты клетки
        char symbol[10]; // символ, с которым сравниваем
        char then_cmd[300]; // команда, которая будет выполнена, если условие истинно
        // Извлекаем координаты, символ и команду THEN
        if (sscanf(line, "IF CELL %d %d IS %9s THEN %[^\n]", &x, &y, symbol, then_cmd) != 4) {
            fprintf(stderr, "Ошибка формата IF.\n");
            exit(1);
        }
        // Проверяем, что координаты в пределах поля
        if (x < 0 || x >= pole_status.shirina || y < 0 || y >= pole_status.height) {
            fprintf(stderr, "Координаты вне поля в IF.\n");
            exit(1);
        }
        // Получаем символ, находящийся в клетке (x, y)
        char current = pole_status.kletki[y][x].obj;  // Прямой доступ OK внутри поля
        // Проверяем, совпадает ли символ в клетке с ожидаемым
        if ((strlen(symbol) == 1 && current == symbol[0]) ||
            (symbol[0] >= 'a' && symbol[0] <= 'z' && current == symbol[0])) {
            // Выполнить then_cmd
            char fake[300]; // буфер для копии команды
            strcpy(fake, then_cmd); // копируем команду
            char* p = fake; // указатель на начало команды
            // Пропускаем начальные пробелы в команде
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p) { // если после пробелов есть команда
                save_status(); // снова сохраняем состояние перед выполнением вложенной команды
                // Рекурсивно вызываем process_line для выполнения команды из THEN
                process_line(p, line_counter, source);
            }
        }
    }
    // Если строка не содержит известной команды
    else {
        fprintf(stderr, "Неизвестная команда: %s\n", cmd);
        exit(1); // завершаем программу с ошибкой
    }

    //выводим текущее состояние поля
    // После выполнения любой команды (кроме EXEC, которая сама вызывает print_pole при возврате)
    // выводим текущее состояние поля в консоль (если включён вывод)
    print_pole();
}


// EXEC - рекурсивное выполнение
void execute_file(const char* filename, int* line_counter) {
    FILE* f = fopen(filename, "r");
    if (!f) { perror("EXEC"); exit(1); }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        (*line_counter)++;
        process_line(line, line_counter, filename);
    }
    fclose(f);
}

// Сохранение результата
void save_output(const char* filename) {
    //проверяем, нужно ли сохранять
    if (!use_save) return; // если флаг use_save == 0 (опция --no-save), функция завершается без сохранения

    //пытаемся открыть файл для записи
    FILE* f = fopen(filename, "w"); //открываем файл с именем filename в режиме записи ("w") - означает сосздай файл заново или почисти его и запиши в него то, что я сейчас напишу
    if (!f) { // если файл не удалось открыть (например, нет прав, или путь неверный)
        perror("output"); // выводим сообщение об ошибке через perror
        return;
    }

    //записываем размеры поля в начало файла
    fprintf(f, "%d %d\n", pole_status.shirina, pole_status.height); // ширина и высота через пробел, затем новая строка

    for (int y = 0; y < pole_status.height; y++) { 
        for (int x = 0; x < pole_status.shirina; x++) {
            char obj = pole_status.kletki[y][x].obj; //obj клетки (например, '#', '@', '_', 'a' и т.д.)

            //если пустая, но есть цвет, выводим цвет
            if (obj == '_' && pole_status.kletki[y][x].color) { //obj пустая клетка '_', но у неё есть цвет (color != 0)
                fputc(pole_status.kletki[y][x].color, f); // записываем в файл символ цвета (например, 'a', 'b', 'c'...)
            } else {
                //иначе символ obj
                fputc(obj, f); // записываем в файл сам символ obj (например, '#', '@', '%', '^' и т.д.)
            }
        }
        fputc('\n', f); // перевод строки в файле, чтобы следующая строка поля шла на новой строке
    }
    fclose(f); // завершаем работу с файлом, освобождаем ресурсы
}

// Парсинг аргументов командной строки
void parse_args(int argc, char* argv[], char** input, char** output) {
    // Проверяем, достаточно ли аргуменнтов передано в командной строке
    if (argc < 3) { //если argc < 3, значит, нет как минимум input.txt и output.txt
        // Выводим сообщение об использовании в stderr (поток ошибок)
        fprintf(stderr, "Использование: %s input.txt output.txt [опции]\n", argv[0]);
        exit(1);
    }

    // Сохраняем имя входного файла
    *input = argv[1]; // argv[1] - второй аргумент командной строки (первый после имени программы)

    // Сохраняем имя выходного файла
    *output = argv[2]; // argv[2] - третий аргумент командной строки

    // Цикл по оставшимся аргументам
    for (int i = 3; i < argc; i++) {
        // Проверяем, является ли текущий аргумент опцией "--no-display"
        if (strcmp(argv[i], "--no-display") == 0) { // strcmp возвращает 0, если строки равны
            use_display = 0;
        }
        // Проверяем, является ли текущий аргумент опцией "--no-save"
        else if (strcmp(argv[i], "--no-save") == 0) { // если строка равна "--no-save"
            use_save = 0;
        }
        // Проверяем, является ли текущий аргумент опцией "--interval"
        else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) { // если равно "--interval" и есть ещё один аргумент после него
            // Увеличиваем i, чтобы перейти к следующему аргументу (числу)
            i++; // теперь i указывает на аргумент после "--interval"
            // Преобразуем строку (число) в целое число и сохраняем в delay_sec
            delay_sec = atoi(argv[i]); // atoi преобразует строку в int
        }
        // Если аргумент не подошёл ни под одну из опций, он игнорируется (или можно добавить ошибку, но пусть так останется)
    }
}
// Главная функция программы
int main(int argc, char* argv[]) {
    // Объявляем переменные для хранения имён входного и выходного файлов
    char* input_file, *output_file;

    // Вызываем функцию для обработки аргументов командной строки
    // Она заполнит input_file и output_file, а также установит глобальные флаги (use_display, use_save, delay_sec)
    parse_args(argc, argv, &input_file, &output_file);

    // Пытаемся открыть файл с именем input_file в режиме чтения ("r")
    FILE* in = fopen(input_file, "r");
    // Проверяем, удалось ли открыть файл
    if (!in) { // если in == NULL, значит, файл не открылся
        perror("input"); // выводим сообщение об ошибке (например, "No such file or directory")
        return 1; // завершаем программу с кодом ошибки
    }

    // Объявляем буфер для чтения одной строки из файла
    char line[512]; // максимальная длина строки - 511 символов + '\0'
    // Объявляем счётчик строк для отслеживания номера текущей строки (нужно для сообщений об ошибках)
    int line_counter = 0;

    // Цикл: читаем строки из файла, пока не достигнем конца файла (EOF) или не произойдёт ошибка
    while (fgets(line, sizeof(line), in)) { // fgets возвращает NULL, если строка не прочиталась
        line_counter++; // увеличиваем счётчик строк
        // Обрабатываем текущую строку: разбираем команду, выполняем её, обновляем поле и тд
        process_line(line, &line_counter, input_file); // передаём строку, счётчик и имя файла (для ошибок)
    }

    // Закрываем входной файл, освобождаем ресурсы
    fclose(in);

    // Вызываем функцию для сохранения итогового состояния поля в выходной файл
    // Она использует имя output_file, а также глобальный флаг use_save
    save_output(output_file);

    // Завершаем программу с кодом успеха вухууууу
    return 0;
}
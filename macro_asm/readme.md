##  Макроассемблер для языка Ассемблера
Фичи:
- Условия
- Циклы
- Макропеременные
- Макроподстановки и макросы (без вложенных макросов)

        value: define 'stringexpression'
        value: define expression
        
        #value ;подстановка полученного значения
        ##value ; подстановка полученного значения в виде строки 
        
        ifdef name ; поиск значения name
        ; code
        else
        ; code
        endif
        
        value: set 'stringexpression'
        value: set expression
        
        ; set -- смена текущего значения
        
        if <expression_bool>
        ;smth
        else
        ; smth
        endif
        
        ifndef name 
        ; code
        else
        ; code
        endif 
        
        rept <expression>
        ; repeat n times
        endr
        
        while <expression_bool>
        ; whileexpression is true
        endw
        
        include 'filename'

Грамматика в РБНФ (первая версия, подстановки не учитываются в грамматике)

    программа = {оператор}
    оператор = {оператор_без_макроса, макрос}
    оператор_без_макроса = {усл_опр, усл_неопр, усл_если, цикл_пока, цикл_повтор, перем, присв, нетерм_строка, вызов_макро, включение}
    макрос = метка "macro" [аргумент_макро | {',' аргумент_макро}] конец_строки {оператор_без_макроса} " endm" конец_строки
    метка = (буква | '_') {буква | '_' | цифра} ':'
    буква = A-Z | a-z | А-Я | а-я
    цифра = 0-9
    конец_строки = '\n'
    аргумент_макро = {любой_символ_юникода}+
    
    нетерм_строка = {любой_символ_юникода} конец_строки
    усл_опр = ' ifdef' выражение конец_строки {оператор} ' endif' конец_строки
    усл_неопр = ' ifndef' выражение конец_строки {оператор} ' endif' конец_строки
    усл_если = ' ifndef' выражение конец_строки {оператор} ' endif' конец_строки
    цикл_пока = ' while' выражение конец_строки {оператор} ' endif' конец_строки
    цикл_повтор = ' rept' выражение конец_строки {оператор} ' endif' конец_строки
    перем = метка 'define' выражение конец_строки
    присв = метка 'set' выражение конец_строки
    включение = ‘ include’ выражение конец_строки
    вызов_макро = идент [выражение | {',' выражение}]
    
    оператор = выражение
    выражение = выр_арифм | выр_лог 
    выр_арифм = ["+"|"-"] ар_терм { ("+"|"-") ар_терм }
    ар_терм = ар_фактор {("*"|"/") ар_фактор }
    ар_фактор = число | строка | "(" выр_арифм ")"
    выр_лог = лог_терм { "or" лог_терм }
    лог_терм = лог_фактор {"and" лог_фактор }
    лог_фактор = "True" | "False" | сравнение | "(" выр_лог ")"
    
    сравнение = выр_арифм оп_сравн выр_арифм 
    оп_сравн = "="|"!="|"<"|">"|"<="|">="
    
    число = число_дв | число_шест | число_цел | число_дроб
    строка = '\'' {любой_символ_юникода} '\''
    число_дв = '0b' {'0' | '1'}+
    число_шест = '0x' {цифра | А-F | a-f}+
    число_цел = {цифра}+
    число_дроб = {цифра}+ [("E"|"e") ["+"|"-"] число_цел]
    

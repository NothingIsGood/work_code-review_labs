# -*- coding: utf-8 -*-
"""
Created on Mon Dec 3 2018

@author: NothingIsGood
"""

from asm_tools import AssemblyOperator
from enum import Enum, unique
'''
class AssemblyError(object):
    def __init__(self, pos: int, message: str):
        self.pos = pos
        self.message = message
    
    def __str__(self):
        return f'Error in Line {self.pos}: \"{self.message}\"'
'''

class ParserTool(object):
    @staticmethod
    def parse_arguments(args: str):
        '''
        Разбивает строку args на последовательность аргументов
        аргумент -- любое выражение (не проверяется синтаксис) или строка
        строки проверяются, так как разделители внутри строк -- просто символы
        '''
        res: list = []
        delim: str = ','
        need_quotemark : bool = False
            
        current_arg: str = ""
            
        for i, elem in enumerate(args):
            # проверка разделителя в строке
            if elem == delim and not need_quotemark:
                res.append(current_arg)
                current_arg = ""
            elif elem == '\'': # встретили кавычку
                need_quotemark = not need_quotemark
                current_arg += elem
            else:
                if not (elem.isspace() and not need_quotemark): #пробельный символ не в строке удаляем за ненужностью
                    current_arg += elem
        
        if need_quotemark:
            raise Exception("'\"\'\" not found!")
        
        res.append(current_arg)

        return res
    
    @staticmethod
    def parse_assembly_string(command_asm: str):
        # делегирование работы конечному автомату для игнорирования синтаксически неверных строк
        res = AssemblyOperator()
        try:
            res = ParserTool.__parse_string(command_asm)
        except Exception as e:
            res = AssemblyOperator()
            res.source = command_asm[:]
            
        return res
        
    
    @staticmethod    
    def __parse_string(command_asm: str):
        '''
        Простой анализатор строки на ассемблере
        Приводит к структуре оператора ассемблера
        Формат = <label:><<space>code><arguments<comma arguments>><;comment>
        '''
        # syntax analyzer for assembly string
        # format <label: ><<space>code><arguments<comma arguments>><;comment>
        
        @unique
        class State(Enum):
            '''
            Состояния конечного автомата
            '''
            START = 1      # Стартовое состояние
            WAITOP = 2     # Ожидание кода операции
            LABEL = 3      # Ввод метки
            NOCOLON = 4    # Отсутствие символа :
            CODEOP = 5      # Ввод кода операции
            WAITARG = 6     # Ожидание аргументов
            ARG = 7        # Ввод аргумента
            FINISH = 8      # Конечное состояние
            ERROR = 9       # Состояние ошибки
        
        class CharClass(Enum):
            '''
            Тип символа -- нужен для конечного автомата
            '''
            BLANK = 1     # пробелы и табуляция
            COLON = 2     # Двоеточие
            COMMENT = 3   # Комментарий
            DIGIT = 4     # Цифра
            ID = 5        # Идентификатор
            UNDERLINE = 6 # Подчеркивание
            ARG = 7       # Аргумент (они же любые символы)
            NOTBLANK = ARG   # Непробельный символ

        def get_char_class(x: str):
            '''
            Возвращает класс символа для конечного автомата
            '''
            if x.isspace():
                return CharClass.BLANK
            if x == ':':
                return CharClass.COLON
            if x == ';':
                return CharClass.COMMENT
            if x.isdigit():
                return CharClass.DIGIT
            if x.isalpha:
                return CharClass.ID
            if x == '_':
                return CharClass.UNDERLINE
            return CharClass.NOTBLANK
        
        
        
        res: AssemblyOperator = AssemblyOperator()
        res.source = command_asm[:]
        
        command_str: str = command_asm[:] #get copy
        
        # первый символ пробел -- у нас метка
        # первый символ не пробел -- у нас код операции
        cur_state = State.START
        parsed_str: str = ""
        
        for i, elem in enumerate(command_str):
            ch: CharClass = get_char_class(elem);
            if cur_state == State.START:
                if ch == CharClass.BLANK:
                    cur_state = State.WAITOP
                elif ch == CharClass.ID or ch == CharClass.UNDERLINE:
                    parsed_str = elem
                    cur_state = State.LABEL
                elif ch == CharClass.COMMENT:
                    res.comment = parsed_str
                    cur_state = State.FINISH
                else:
                    cur_state = State.ERROR
            elif cur_state == State.WAITOP:
                if ch == CharClass.BLANK:
                    cur_state = State.WAITOP
                elif ch == CharClass.ID:
                    parsed_str = elem
                    cur_state = State.CODEOP
                elif ch == CharClass.COMMENT: # дошли до комментария
                    if not res.label:
                        res.work = False
                    res.comment = parsed_str
                    cur_state = State.FINISH
                    
                else:
                    cur_state = State.ERROR
            elif cur_state == State.LABEL:
                if ch == CharClass.ID or ch == CharClass.UNDERLINE or ch == CharClass.DIGIT: # Допустимый символ
                    parsed_str += elem
                    cur_state = State.LABEL
                elif ch == CharClass.COLON: # Конец метки
                    res.label = parsed_str
                    parsed_str = ""
                    cur_state = State.WAITOP
                elif ch == CharClass.BLANK: # пробел без конца метки
                    res.label = parsed_str
                    parsed_str = ""
                    cur_state = State.NOCOLON
                else:
                    cur_state = State.ERROR
            elif cur_state == State.CODEOP:
                if ch == CharClass.ID: # Допустимый символ
                    parsed_str += elem
                    cur_state = State.CODEOP
                elif ch == CharClass.BLANK: # Конец кода операции -- ждем операнды
                    res.code = parsed_str
                    parsed_str = ""
                    cur_state = State.WAITARG
                elif ch == CharClass.COMMENT: # Дошли до комментария
                    res.code = parsed_str
                    parsed_str = ""
                    res.comment = command_str[i:]
                    cur_state = State.FINISH
                else:
                    cur_state = State.ERROR
            elif cur_state == State.WAITARG:
                if ch == CharClass.BLANK:
                    cur_state = State.WAITARG # Считаем пробелы
                elif ch == CharClass.COMMENT: # Дождались комментария
                    res.comment = command_str[i:]
                    cur_state = State.FINISH
                else: # Аргумент
                    parsed_str = elem
                    cur_state = State.ARG
            elif cur_state == State.ARG:
                if ch == CharClass.COMMENT: # Комментарий
                    res.arguments = ParserTool.parse_arguments(parsed_str)
                    parsed_str = "";
                    res.comment = command_str[i:]
                    cur_state = State.FINISH
                
                else: # Считаем аргумент
                    parsed_str += elem
                    cur_state = State.ARG
            elif cur_state == State.FINISH:
                return res
            elif cur_state == State.ERROR:
                raise Exception("Invalid char")
                res.work = False
                res.comment = parsed_str
                return res
            elif cur_state == State.NOCOLON:
                raise Exception("Invalid label")
                cur_state = State.WAITOP
            
        # Дозапись при конце строки в заданных состояниях
        if cur_state == State.CODEOP:
            res.code = parsed_str
        elif cur_state == State.ARG:
            res.arguments = ParserTool.parse_arguments(parsed_str)
        elif cur_state == State.LABEL:
            raise Exception("Invalid operator")
            res.comment = command_str;
    
        for elem in res.arguments:
            if len(elem) == 0:
                raise Exception("Empty arguments")
                    
        return res

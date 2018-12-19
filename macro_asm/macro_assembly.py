# -*- coding: utf-8 -*-
"""
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
    
    ipython launch = runfile('C:/clion_proj/code-review_labs/macro_asm/macro_assembly.py', wdir='C:/clion_proj/code-review_labs/macro_asm', args='macro_test.txt out.txt')
    
"""
from expression_eval import eval_expr
from parser_tools import ParserTool
from asm_tools import MacroCode

import copy # для глубокого копирования
import sys

MACRO_TOKEN = "macro"
ENDM_TOKEN = "endm"
IFDEF_TOKEN = "ifdef"
ENDIF_TOKEN = "endif"
ELSE_TOKEN = "else"
DEFINE_TOKEN = "define"
SET_TOKEN = "set"
IFNDEF_TOKEN = "ifndef"
REPT_TOKEN = "rept"
ENDR_TOKEN = "endr"
WHILE_TOKEN = "while"
ENDW_TOKEN = "endw"
IF_TOKEN = "if"
INCLUDE_TOKEN = "include"



class MacroAssembly(object):
    def __init__(self, filename : str = "", filename_out : str = ""):
        '''
        @param filename -- имя файла с кодом на языке ассемблера
        all_file = вектор строк
        macro_table = таблица макроопределений
        output_file = выходной файл (вектор строк)
        
        '''
        self.filename = filename
        self.filename_out = filename_out
        self.all_file = [] # список структур -- операторов ассемблера
        self.macro_table = {} # dict <str, MacroOperator>
        self.output_file = [] 
        self.variables = {} # таблица макропеременных
        
    def __open_file(self):
        '''
        Инициализация файла с кодом
        '''
        with open(self.filename, 'r') as fs:
            self.all_file = [ParserTool.parse_assembly_string(asm_str) for asm_str in fs.readlines()]
        
    
    def parse_macro(self, idx: int):
        '''
        Логика для команды macro
        Выполнение директив и проч
        
        @param idx - строка для обработки (номер в файле)
        '''
        macro_header = self.all_file[idx]
        res = MacroCode(macro_header.label, macro_header.arguments)
        res.data = []
        
        idx += 1 #next element
        current = self.all_file[idx]
        
        while idx < len(self.all_file) and current.code != ENDM_TOKEN:
            res.data.append(self.all_file[idx])
            idx += 1
            if idx < len(self.all_file):
                current = self.all_file[idx]
            
        if idx == len(self.all_file): raise Exception(f"endm not found for macro \"{res.name}\"")
        
        self.macro_table[macro_header.label] = res
        
        return idx
        
    def work(self):
        '''
        Основной цикл работы макропроцессора
        '''
        try:
            self.__open_file()
            idx = 0
            idx = self.statements(idx)
        except Exception as e:
            ex_str = f'Macro Assembly Error: {e}'
            if self.output_file:
                self.output_file[-1] += f";{ex_str}"
            else:
                self.output_file.append(ParserTool.parse_assembly_string(f";{ex_str}"))
            print(ex_str)
            
        output_filename = self.filename_out
        with open(output_filename, 'w') as fs:
            for str_asm in self.output_file:
                fs.write(str_asm)
                #print(str_asm, end = '')
        print("MacroAssembler finished his work")
            
    def parse_ifdef(self, pos: int):
        '''
        Обработка конструкции ifdef
        грамматика вида ifdef <name> <statements> <else> <statements> endif
        ветка else не обязательна
        
        '''
        return_pos = pos + 1
        
        if len(self.all_file[pos].arguments) != 1:
            raise Exception('invalid num params')
        expr_result = eval_expr(ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0]) in self.variables
    
        while return_pos < len(self.all_file) and self.all_file[return_pos].code != ENDIF_TOKEN:
            if self.all_file[return_pos].code == ELSE_TOKEN:
                expr_result = not expr_result
                return_pos += 1
                continue
            
            if expr_result:
                return_pos = MacroAssembly.statement(self, return_pos)
            else:
                return_pos += 1
        
        if return_pos == len(self.all_file):
            raise Exception('endif not found')
                
        return return_pos
    
    def parse_ifndef(self, pos: int):
        '''
        Обработка конструкции ifndef
        грамматика вида ifndef <name> <statements> <else> <statements> endif
        ветка else не обязательна
        
        '''
        return_pos = pos + 1
        
        if len(self.all_file[pos].arguments) != 1:
            raise Exception('invalid num params')
        expr_result = eval_expr(ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0]) not in self.variables
    
        while return_pos < len(self.all_file) and self.all_file[return_pos].code != ENDIF_TOKEN:
            if self.all_file[return_pos].code == ELSE_TOKEN:
                expr_result = not expr_result
                return_pos += 1
                continue
            
            if expr_result:
                return_pos = MacroAssembly.statement(self, return_pos)
            else:
                return_pos += 1
        
        if return_pos == len(self.all_file):
            raise Exception('endif not found')
                
        return return_pos
    
    def parse_if(self, pos: int):
        '''
        Обработка конструкции if
        грамматика вида ifndef <expr> <statements> <else> <statements> endif
        ветка else не обязательна
        
        '''
        return_pos = pos + 1
        
        if len(self.all_file[pos].arguments) != 1:
            raise Exception('invalid num params')
        expr_result = bool(eval_expr(ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0]))
    
        while return_pos < len(self.all_file) and self.all_file[return_pos].code != ENDIF_TOKEN:
            if self.all_file[return_pos].code == ELSE_TOKEN:
                expr_result = not expr_result
                return_pos += 1
                continue
            
            if expr_result: # если true -- то обрабатываем
                return_pos = MacroAssembly.statement(self, return_pos)
            else: # иначе обходим
                return_pos += 1
        
        if return_pos == len(self.all_file):
            raise Exception('endif not found')
                
        return return_pos
    
    def parse_rept(self, pos: int):
        '''
        Логика обработки директивы rept
        rept (целое значение) -- выполнение n раз
        '''
        return_pos = pos + 1
        
        if len(self.all_file[pos].arguments) != 1:
            raise Exception('invalid num params')
            
        expr_str = ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0][:]
        n = int(eval_expr(expr_str))
        
        while n:
            while return_pos < len(self.all_file) and self.all_file[return_pos].code != ENDR_TOKEN:
                return_pos = MacroAssembly.statement(self, return_pos)
                    
            if return_pos == len(self.all_file):
                raise Exception('endw not found')
            
            n -= 1
            
            if n: # переходим только если не прошли n раз
                return_pos = pos + 1
      
        return return_pos
    
    def parse_while(self, pos: int):
        '''
        Логика обработки директивы while
        '''
        return_pos = pos + 1
        
        if len(self.all_file[pos].arguments) != 1:
            raise Exception('invalid num params')
        
        expr_str = ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0][:]
        expr_result = bool(eval_expr(expr_str))
        
        cycle_pred = True # устанавливается в True при первом проходе по тексту 
        
        
        while cycle_pred == True:
            while return_pos < len(self.all_file) and self.all_file[return_pos].code != ENDW_TOKEN:
                if expr_result:
                    return_pos = MacroAssembly.statement(self, return_pos)
                else: 
                    return_pos += 1
                    
            if return_pos == len(self.all_file):
                raise Exception('endw not found')
            
            if expr_result == False:
                cycle_pred = expr_result
            else:
                expr_str = ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0][:]
                cycle_pred = expr_result = bool(eval_expr(expr_str))
        
                if expr_result != False:
                    return_pos = pos + 1
      
        return return_pos
        
    def parse_define(self, pos: int):
        '''
        Логика обработки директивы define
        '''
        label: str = self.all_file[pos].label
        if label not in self.variables.keys():
            if len(self.all_file[pos].arguments) == 1:
                self.variables[label] = eval_expr(ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0])
            else:
                raise Exception("Invalid number of params")
        else:
            raise Exception(f"variable {label} has already defined")
        return pos
        
    def parse_set(self, pos: int):
        '''
        Логика обработки директивы set
        '''
        label: str = self.all_file[pos].label
        if label in self.variables.keys():
            if len(self.all_file[pos].arguments) == 1:
                self.variables[label] = eval_expr(ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0])
            else:
                raise Exception("Invalid number of params")
        else:
            raise Exception(f"variable {label} has never defined")
        return pos
    
    def parse_include(self, pos: int):
        '''
        Логика обработки include -- удаляем
        '''
        if len(self.all_file[pos].arguments) != 1:
            raise Exception('invalid num params')
            
        filename = str(eval_expr(ParserTool.parse_assembly_string(self.__check_defines(pos)).arguments[0]))
        
        self.all_file = self.all_file[:pos] + [ParserTool.parse_assembly_string(s) for s in open(filename).readlines()] + self.all_file[pos + 1:]
        
        return pos - 1
    
    def __check_defines(self, pos: int):
        '''
        Выполнение подстановки в исходнную строку файла
        
        Подстановка уровня 2 -- возвращает строковое представление (обрамлено кавычками)
        Подстановка уровня 1 -- возвращает текущее значение
        '''
        
        # подстановка уровня 2
        
        res = copy.deepcopy(self.all_file[pos].source)
        for key in self.variables.keys():
            replace_name = '##' + key
            if res.find(replace_name) != -1:
                res = res.replace(replace_name, f'\'str(self.variables[key])\'')
        
        # подстановка уровня 1
        for key in self.variables.keys():
            replace_name = '#' + key
            if res.find(replace_name) != -1:
                res = res.replace(replace_name, str(self.variables[key]))
                
        return res
    
    
    def statement(self, pos: int):
        '''
        Обработка конструкции statement
        
        '''
        
        # обработка подстановок для строк (переменных)
        # обработка макроподстановок
        
        #print(self.variables)
        #print(self.all_file[pos].source)
        
        work_str = copy.deepcopy(self.all_file[pos])
        
        work_str.source = self.__check_defines(pos)
        work_str = ParserTool.parse_assembly_string(work_str.source)
        
        return_pos = pos
        
        if work_str.code == IFDEF_TOKEN:
            return_pos = self.parse_ifdef(pos)
        elif work_str.code == IF_TOKEN:
            return_pos = self.parse_if(pos)
        elif work_str.code == IFNDEF_TOKEN:
            return_pos = self.parse_ifndef(pos)
        elif work_str.code == MACRO_TOKEN:
            return_pos = self.parse_macro(pos)
        elif work_str.code == DEFINE_TOKEN:
            return_pos = self.parse_define(pos)
        elif work_str.code == SET_TOKEN:
            return_pos = self.parse_set(pos)
        elif work_str.code == WHILE_TOKEN:
            return_pos = self.parse_while(pos)
        elif work_str.code == REPT_TOKEN:
            return_pos = self.parse_rept(pos)
        elif work_str.code == INCLUDE_TOKEN:
            return_pos = self.parse_include(pos)
        elif work_str.code in self.macro_table.keys():
            self.output_file += MacroAssembly.evaluate([ParserTool.parse_assembly_string(asm_op) 
                                                        for asm_op in self.macro_table[work_str.code]
                                                        .execute(work_str.arguments)],
                                                        self.macro_table, self.variables)
        else:
            
            self.output_file.append(work_str.source)
        return return_pos + 1

    def statements(self, pos):
        '''
        Стартовое правило грамматики -- statements
        '''
        return_pos = pos
        while return_pos < len(self.all_file):
            return_pos = self.statement(return_pos)
        return return_pos
    
    @staticmethod
    def evaluate(source: list, macro_names: dict, variables: dict):
        '''
        Основной алгоритм интерпретирования макроассемблерного кода
        @param source -- вектор строк (структура оператора ассемблера)
        @param macro_names -- таблица макросов
        @param variables -- таблица переменных
        '''
        res: list = []
        tmp_masm = MacroAssembly()
        tmp_masm.all_file = source
        tmp_masm.macro_table = macro_names
        tmp_masm.variables = variables
        tmp_masm.output_file = res
        pos = 0
        tmp_masm.statements(pos)
        return tmp_masm.output_file[:]
            
    
if __name__ == "__main__":
    if len(sys.argv) == 3:
        filename_source = sys.argv[1]
        filename_dest = sys.argv[2]
        
        m = MacroAssembly(filename_source, filename_dest)
        m.work()
    else:
        print(f'Error: Invalid number of args: {len(sys.argv)}')
    
    
    #print(m.output_file)
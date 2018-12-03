# -*- coding: utf-8 -*-
"""
Created on Mon Dec 3 2018

@author: NothingIsGood
"""

class AssemblyOperator(object):
    '''
    Структура для оператора ассемблера (в 3.7 = dataclass)
    '''
    def __init__(self, label: str = "", code: str = "", arguments: list = [], comment: str = "", source: str = ""):
        '''
        @param label метка ассемблера
        @param code код операции ассемблера
        @param arguments аргументы
        @param comment комментарий
        @param work подлежит ли обработке
        @param source исходная строка для обработки
        '''
        
        self.label = label
        self.code = code
        self.arguments = arguments
        self.comment = comment
        self.work = True
        self.source = source
        
    def __str__(self):
        return f'label: {self.label}\ncode: {self.code}\nargs: {self.arguments}'

class MacroCode(object):
    '''
    Макросы хранятся здесь
    '''
    def __init__(self, name: str = "", params: list = [], data: list = []):
        '''
        macro param1, param2, param3
         code1 %param1
         code2 %param2 + %param3
        endm
        
        @param name - название макроса
        @param params - параметры 
        @param data - строки на макроассемблере
        '''
        self.name = name
        self.params = params
        self.data = data
        
    def execute(self, args: list):
       '''
       Возвращает массив строк после обработки макроассемблера
       @param args список аргументов для подстановки
       
       Выполнение директив не здесь, а в логике макроассемблере
       '''
       if len(args) != len(self.params):
           raise Exception("Invalid number of params")
           
       res: list = [] 
       # подстановка параметров
       for elem in self.data:
           tmp: str = elem.source[:]
           for i, param in enumerate(self.params):
               tmp = tmp.replace('%' + param, args[i])
           res.append(tmp)
       # интерпретация
       return res
        
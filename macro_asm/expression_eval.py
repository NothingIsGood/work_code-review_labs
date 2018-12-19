# -*- coding: utf-8 -*-
"""
Created on Mon Dec 3 2018

@author: NothingIsGood
"""

import ast
import operator as op

# supported operators
operators = {ast.Add: op.add, ast.Sub: op.sub, ast.Mult: op.mul,
             ast.Div: op.truediv, ast.USub: op.neg, ast.Gt: op.gt,
             ast.GtE: op.ge, ast.Lt: op.lt, ast.LtE: op.le, ast.Eq: op.eq,
             ast.NotEq: op.ne, ast.And: op.and_, ast.Or: op.or_}

def eval_expr(expr):
    return eval_(ast.parse(expr, mode='eval').body)

def eval_(node):
    '''
    safe eval() variant
    support:
        numbers
        strings
        boolean expressions (and, or)
        math expressions (+, -, *, /)
        compare (>, >=, <, <=, ==, !=)
    '''
    if isinstance(node, ast.Num): # <number>
        return node.n
    if isinstance(node, ast.Str):
        return node.s
    elif isinstance(node, ast.BinOp): # <left> <operator> <right>
        return operators[type(node.op)](eval_(node.left), eval_(node.right))
    elif isinstance(node, ast.UnaryOp): # <operator> <operand> e.g., -1
        return operators[type(node.op)](eval_(node.operand))
    elif isinstance(node, ast.BoolOp): # <left> <operator> <right>
        return operators[type(node.op)](eval_(node.values[0]), eval_(node.values[1]))
    elif isinstance(node, ast.Compare): # <left> <operator> <right>
        left = eval_(node.left)
        right = eval_(node.comparators[0])
        _op = operators[type(node.ops[0])]
        return _op(left, right)
    else:
        raise Exception("Invalid Operation in macroexpression")


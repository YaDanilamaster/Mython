# Описание языка Mython

Mython представляет собой DSL (domain-specific language — предметно-ориентированный язык).
В Mython есть классы и наследование, а все методы — виртуальные.

Mython состоит из трех основных блоков:
- parse - лексический парсер
- runtime - runtime-модуль, отвечающий за управление состоянием программы во время её работы
- ast - абстрактное синтаксическое дерево

Общая схема выглядит так:

![Mython diagram](https://github.com/YaDanilamaster/Mython/blob/main/Mython%20diagram.jpg?raw=true)

## Синтаксис и семантика Mython

**`Числа`** В языке Mython используются только целые числа. С ними можно выполнять обычные арифметические операции: сложение, вычитание, умножение, целочисленное деление.

**`Строки`** 
Строковая константа в Mython — это последовательность произвольных символов, размещающаяся на одной строке и ограниченная двойными кавычками " или одинарными '. Поддерживается экранирование спецсимволов '\n', '\t', '\'' и '\"'

**`Логические константы и None`** 
Кроме строковых и целочисленных значений язык Mython поддерживает логические значения True и False. Есть также специальное значение None, аналог nullptr в С++. В отличие от C++, логические константы пишутся с большой буквы.

**`Комментарии`** 
Mython поддерживает однострочные комментарии, начинающиеся с символа #.

**`Идентификаторы`**
Идентификаторы в Mython используются для обозначения имён переменных, классов и методов. Об этом далее. Идентификаторы формируются так же, как в большинстве других языков программирования: начинаются со строчной или заглавной латинской буквы либо с символа подчёркивания. Потом следует произвольная последовательность, состоящая из цифр, букв и символа подчёркивания.

**`Классы`**
В Mython можно определить свой тип, создав класс. Как и в С++, класс имеет поля и методы, но, в отличие от С++, поля не надо объявлять заранее.
Объявление класса начинается с ключевого слова class, за которым следует идентификатор имени и объявление методов класса. Пример класса «Прямоугольник»:
```Python
class Rect:
  def __init__(w, h):
    self.w = w
    self.h = h

  def area():
    return self.w * self.h
```

**`Важные отличия классов в Mython от классов в C++:`**
Специальный метод __init__ играет роль конструктора — он автоматически вызывается при создании нового объекта класса. Метод __init__ может отсутствовать.
Неявный параметр всех методов — специальный параметр self, аналог указателя this в C++. Параметр self ссылается на текущий объект класса.
Поля не объявляются заранее, а добавляются в объект класса при первом присваивании. Поэтому обращения к полям класса всегда надо начинать с self., чтобы отличать их от локальных переменных.
Все поля объекта — публичные.
Новый объект ранее объявленного класса создаётся так же, как в C++: указанием имени класса, за которым в скобках идут параметры, передаваемые методу __init__.

**`Типизация`**
В отличие от C++, Mython — это язык с динамической типизацией. В нём тип каждой переменной определяется во время исполнения программы и может меняться в ходе её работы.

**`Операции`**
В Mython определены:
- Арифметические операции для целых чисел, деление выполняется нацело. Деление на ноль вызывает ошибку времени выполнения.
- Операция конкатенации строк, например: s = 'hello, ' + 'world'.
- Операции сравнения строк и целых чисел ==, !=, <=, >=, <, >; сравнение строк выполняется лексикографически.
- Логические операции and, or, not.
- Унарный минус.

Приоритет операций (в порядке убывания приоритета):
- Унарный минус.
- Умножение и деление.
- Сложение и вычитание.
- Операции сравнения.
- Логические операции.

**`Команда print`**
Специальная команда print принимает набор аргументов, разделённых запятой, печатает их в стандартный вывод и дополнительно выводит перевод строки.

**`Условный оператор`**
В Mython есть условный оператор. Его синтаксис:
```Python
if <условие>:
  <действие 1>
  <действие 2>
  ...
  <действие N>
else:
  <действие 1>
  <действие 2>
  ...
  <действие M> 
```

**`Наследование`**
В языке Mython у класса может быть один родительский класс. Если он есть, он указывается в скобках после имени класса и до символа двоеточия. В примере ниже класс Rect наследуется от класса Shape:
```Python
class Shape:
  def __str__():
    return "Shape"

  def area():
    return 'Not implemented'

class Rect(Shape):
  def __init__(w, h):
    self.w = w
    self.h = h

  def __str__():
    return "Rect(" + str(self.w) + 'x' + str(self.h) + ')'

  def area():
    return self.w * self.h 
```

**`Семантика присваивания`**
Как сказано выше, Mython — это язык с динамической типизацией, поэтому операция присваивания имеет семантику не копирования значения в область памяти, а связывания имени переменной со значением. Как следствие, переменные только ссылаются на значения, а не содержат их копии. Говоря терминологией С++, переменные в Mython — указатели.

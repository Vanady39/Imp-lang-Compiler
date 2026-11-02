# Модуль семантического анализа

## Назначение

Модуль выполняет семантический анализ AST после успешных лексического и синтаксического анализов.

Основные задачи модуля:

- обход всех узлов AST;
- управление областями видимости;
- регистрация переменных, типов, параметров и рутин;
- разрешение типов выражений;
- вызов атомарных checker-сервисов;
- накопление семантических ошибок;
- проверка контекста оператора `return`.

Модуль не изменяет AST. Он анализирует уже построенное дерево и сохраняет найденные типы выражений в `CheckResult`.

## Подключение к compiler pipeline

Запуск выполняется в `runners/runner.cpp` после лексического и синтаксического анализа:

```text
исходный файл
    ↓
lexer
    ↓
parser
    ↓
semantic checker
    ↓
ast printer
```

Если lexer или parser нашли ошибку, семантический анализ не запускается.

Если семантический анализ нашёл ошибки, они выводятся в stdout, а `runSourceFile` возвращает код `1`. AST печатается только при отсутствии семантических ошибок.

Основная точка входа:

```cpp
unique_ptr<IChecker> checker = makeChecker();
checker->check(*program);
const vector<string>& errors = checker->get_errors();
```

## Структура модуля

| Путь | Назначение |
| --- | --- |
| `checker/checker.h` | публичный интерфейс главного анализатора |
| `checker/checker.cpp` | координация обхода и атомарных проверок |
| `traversal/ast_traversal.h` | интерфейс обхода AST и observer-событий |
| `traversal/ast_traversal.cpp` | рекурсивный обход узлов AST |
| `checker/variable/` | проверки переменных, ссылок, инициализации и простых присваиваний |
| `checker/routine/` | проверки объявлений и вызовов рутин |
| `checker/for-loop/` | проверки числовых и массивных циклов `for` |
| `checker/condition/` | проверка условий `if` |
| `checker/while/` | проверка условий `while` |
| `checker/return/` | проверка операторов `return` |
| `common/helpers/expression_type.*` | разрешение типов выражений и типов AST |
| `types/scope_stack.h` | стек областей видимости |
| `types/symbol.h` | описание символов таблицы имён |

## Общая архитектура

Модуль разделён на два уровня:

1. `AstTraversal` отвечает за порядок обхода и время жизни областей видимости.
2. `Checker` получает события обхода и вызывает нужные атомарные сервисы.

Такое разделение позволяет менять порядок обхода независимо от реализации отдельных проверок. Атомарный checker получает уже подготовленный AST-узел и текущий `ScopeStack`.

### Интерфейс обхода

`IAstTraversal` предоставляет:

```cpp
virtual void traverse(
    const Program& program,
    IAstTraversalObserver& observer
) = 0;
```

`IAstTraversalObserver` получает четыре события:

```cpp
virtual void enter(const Node& node, ScopeStack& scopes) = 0;
virtual void leave(const Node& node, ScopeStack& scopes) = 0;
virtual void enterScope(const Node& node, ScopeStack& scopes) = 0;
virtual void leaveScope(const Node& node, ScopeStack& scopes) = 0;
```

`enter` и `leave` относятся к конкретному AST-узлу. `enterScope` и `leaveScope` обозначают создание и удаление специальной области видимости.

## Как работает обход AST

Обход реализован в `traversal/ast_traversal.cpp` классом `AstTraversal`.

Алгоритм для обычного узла:

1. вызвать `observer->enter(node, scopes)`;
2. если узел создаёт специальную область, вызвать `enterScope`;
3. рекурсивно обойти дочерние узлы;
4. вызвать `observer->leave(node, scopes)`;
5. если узел создаёт специальную область, вызвать `leaveScope`.

Дети обходятся слева направо в порядке, заданном AST. `nullptr` пропускается, поэтому корректно обрабатываются отсутствующие `else`, инициализаторы, размеры массивов, тела прототипов и другие необязательные части дерева.

### `enterNode`

```cpp
void enterNode(const Node& node)
```

Передаёт observer событие входа в узел. В этот момент `Checker` проверяет текущую конструкцию до обхода её детей.

### `leaveNode`

```cpp
void leaveNode(const Node& node)
```

Передаёт observer событие выхода из узла. `Checker` использует его для выхода из контекста текущей рутины.

### `enterScope`

```cpp
void enterScope(const Node& node)
```

Добавляет область в `ScopeStack`, затем сообщает об этом observer:

```cpp
scopes.push();
observer->enterScope(node, scopes);
```

Порядок важен: обработчик получает уже созданную область и может зарегистрировать локальные символы.

### `leaveScope`

```cpp
void leaveScope(const Node& node)
```

Сообщает observer о выходе, затем удаляет область:

```cpp
observer->leaveScope(node, scopes);
scopes.pop();
```

После этого локальные имена больше не видны.

### `traverseNode`

```cpp
void traverseNode(const Node* node)
```

Главный диспетчер обхода.

Метод:

- пропускает `nullptr`;
- отдельно обрабатывает `Program`;
- передаёт `Body` в `traverseBody`;
- вызывает `enterNode` для текущего узла;
- создаёт специальные области рутины и `for`;
- выбирает обработчик по `NodeType`;
- вызывает `leaveNode`;
- удаляет специальные области.

Для выбора обработчика используется `switch` по `NodeType`, как в AST-принтере проекта.

### `traverse`

```cpp
void traverse(
    const Program& program,
    IAstTraversalObserver& traversalObserver
)
```

Сохраняет observer и запускает обход с корня `Program`:

```cpp
observer = &traversalObserver;
traverseNode(&program);
observer = nullptr;
```

### `get_scopes`

```cpp
const ScopeStack& get_scopes() const
```

Возвращает стек областей видимости, используемый обходом. В основном анализаторе переданный стек принадлежит `Checker`, поэтому обход и checker-сервисы работают с одним объектом.

### `makeAstTraversal`

```cpp
unique_ptr<IAstTraversal> makeAstTraversal(ScopeStack& scopes)
```

Создаёт обход, привязанный к переданному `ScopeStack`.

Обход не владеет стеком. Он хранит ссылку на него.

## Порядок обхода конструкций

### `Program`

`Program` создаёт глобальную область:

```text
push global scope
enter Program
    declarations from left to right
leave Program
pop global scope
```

В этой области регистрируются верхнеуровневые переменные, типы и рутины.

### `Body`

Каждый `Body` создаёт блочную область:

```text
push body scope
enter Body
    items from left to right
leave Body
pop body scope
```

Это обеспечивает область жизни локальных переменных в `if`, `while`, `for` и телах рутин.

### `RoutineDeclaration`

Для рутины порядок такой:

```text
enter RoutineDeclaration in outer scope
push routine scope
register parameters
traverse parameters, return type and body
leave RoutineDeclaration
pop routine scope
```

Сама рутина регистрируется во внешней области до обхода тела. Параметры регистрируются во внутренней области рутины.

### `ForLoop`

Для цикла:

```text
check range in current outer scope
enter ForLoop
push loop scope
register loop variable as read-only
traverse loop body
leave ForLoop
pop loop scope
```

Переменная цикла видна внутри тела и исчезает после выхода из цикла.

### Выражения

Обход выражений выполняется рекурсивно:

- `UnaryExpression` — один операнд;
- `BinaryExpression` — левый, затем правый операнд;
- `MemberAccess` — объект доступа;
- `IndexAccess` — массив, затем индекс;
- `RoutineCall` — аргументы по порядку;
- литералы и `NameReference` не имеют дочерних узлов.

Например, для:

```text
x := a + b * 2
```

порядок посещения будет:

```text
Assignment
    NameReference x
    BinaryExpression +
        NameReference a
        BinaryExpression *
            NameReference b
            IntegerLiteral 2
```

## Области видимости

`ScopeStack` содержит вложенные таблицы имён.

| Конструкция | Область |
| --- | --- |
| `Program` | глобальная |
| `RoutineDeclaration` | параметры и контекст рутины |
| `Body` | локальные объявления блока |
| `ForLoop` | read-only переменная цикла |

`if` и `while` не создают область напрямую. Их `thenBody`, `elseBody` и тело цикла представлены `Body`, который создаёт область самостоятельно.

Вложенная область ищет имя сначала у себя, затем во внешних областях. Поэтому локальное объявление может затенять внешнее имя.

Регистрация переменной выполняется после проверки её объявления. Это сохраняет правило объявления до использования:

```text
var x : integer is 10
print x
```

Имя `x` доступно после объявления. Ссылка до объявления вызывает ошибку.

## Что делает `checker.cpp`

`checker.cpp` содержит класс `Checker`, который является главным координатором семантического анализа.

Класс реализует два интерфейса:

```cpp
class Checker final : public IChecker, public IAstTraversalObserver
```

- `IChecker` предоставляет общий API анализатора;
- `IAstTraversalObserver` принимает события от обхода.

`Checker` не обходит AST самостоятельно. Он получает события от `AstTraversal` и запускает необходимые проверки.

## Поля `Checker`

### `errors`

```cpp
vector<string> errors;
```

Накопленные семантические ошибки. Формат сообщений:

```text
Semantic error (line N): <message>
```

### `scopeStack`

```cpp
ScopeStack* scopeStack = nullptr;
```

Указатель на текущий стек областей. Реальный `ScopeStack` создаётся внутри `check` и живёт до конца анализа.

Такой срок жизни нужен, потому что checker-сервисы хранят ссылки на стек.

### `result`

```cpp
CheckResult result;
```

Содержит карту типов выражений:

```cpp
unordered_map<Expression*, TypeInfo> expressionTypes;
```

### Атомарные сервисы

`Checker` владеет сервисами через `unique_ptr`:

| Поле | Проверки |
| --- | --- |
| `variableChecker` | объявления, ссылки, инициализаторы и простые присваивания |
| `routineChecker` | сигнатуры, прототипы и вызовы рутин |
| `forChecker` | числовые и массивные диапазоны |
| `returnChecker` | оператор `return` |
| `conditionChecker` | условия `if` |
| `whileChecker` | условия `while` |
| `typeResolver` | типы выражений и типов |

Каждый сервис остаётся отдельным и получает только необходимые данные.

### `routineStack`

```cpp
vector<const RoutineDeclaration*> routineStack;
```

Хранит текущий контекст рутин.

При входе в `RoutineDeclaration` рутина добавляется в стек. При выходе удаляется. Это позволяет `returnChecker` определить, находится ли `return` внутри рутины и какой результат ожидается.

### `ownedTypes`

```cpp
vector<unique_ptr<TypeInfo>> ownedTypes;
```

Хранит созданные типы переменных. `Symbol` содержит указатель на `TypeInfo`, поэтому объекты типов должны жить до конца анализа.

### `loopTypes`

```cpp
unordered_map<const ForLoop*, const TypeInfo*> loopTypes;
```

Временно сохраняет тип переменной цикла между проверкой диапазона и входом в loop-scope.

Для числового цикла это `integer`. Для цикла по массиву — тип элемента массива.

## Методы `Checker`

### `resetResolvers`

Создаёт резолверы, привязанные к текущему `ScopeStack` и общему списку ошибок.

Вызывается после создания стека перед началом обхода.

### `resolveType`

Безопасно разрешает `TypeNode`. Для `nullptr` возвращает `nullptr`, иначе вызывает `typeResolver->resolveType`.

### `registerSymbol`

Создаёт `Symbol` и добавляет его в текущую область.

Используется для:

- переменных;
- параметров;
- типов;
- переменных циклов.

### `registerRoutine`

Проверяет объявление рутины через `routineChecker`, затем регистрирует рутину или обновляет существующий символ прототипа.

Поддерживаются:

- прототипы;
- определения после прототипа;
- рекурсивные вызовы;
- проверка одинаковых сигнатур;
- обнаружение повторного определения.

Рутина добавляется во внешний scope до обхода тела, поэтому рекурсивный вызов видит собственное имя.

### `checkExpression`

Разрешает тип выражения и записывает его в `result.expressionTypes`.

Для `RoutineCall` дополнительно:

1. разрешает типы аргументов;
2. вызывает `routineChecker->checkCallRoutine`;
3. проверяет количество и совместимость аргументов.

### `checkVariable`

Проверяет переменную:

1. разрешает явно объявленный тип;
2. разрешает тип инициализатора;
3. вызывает `checkInitializer`;
4. получает итоговый или выведенный тип;
5. сохраняет тип в `ownedTypes`;
6. регистрирует символ переменной.

### `checkType`

Разрешает описание `TypeDeclaration` и регистрирует именованный тип.

Регистрация выполняется до следующих деклараций, чтобы они могли использовать этот тип.

### `checkAssignment`

Разрешает тип правой части и проверяет простую цель через `variableChecker`.

Проверяются существование имени, read-only статус и совместимость типов.

Составные цели вроде `record.field` и `array[index]` пока требуют отдельного checker-сервиса.

### `checkCallStatement`

Передаёт вызов в общий `checkExpression`, чтобы вызовы одинаково проверялись в выражениях и операторах.

### `check`

Главный метод анализатора:

1. очищает ошибки и предыдущий результат;
2. создаёт `ScopeStack`;
3. привязывает к нему checker-сервисы;
4. очищает временные контексты;
5. создаёт `AstTraversal`;
6. запускает обход `Program`;
7. возвращает `CheckResult`.

### `get_errors`

Возвращает список семантических ошибок для `runner.cpp`.

## Обработка событий observer

### `enter`

В зависимости от `NodeType` вызывает соответствующую проверку:

| Узел | Действие |
| --- | --- |
| `VariableDeclaration` | `checkVariable` |
| `TypeDeclaration` | `checkType` |
| `RoutineDeclaration` | проверка и регистрация рутины |
| `NameReference` | проверка видимости имени |
| `Assignment` | проверка присваивания |
| `CallStatement` | проверка вызова |
| `IfStatement` | проверка boolean-условия |
| `WhileLoop` | проверка boolean-условия |
| `ForLoop` | проверка диапазона и сохранение типа loop-переменной |
| выражение | разрешение типа и проверка вызова рутины |
| `ReturnStatement` | вызов `returnChecker` |

### `leave`

При выходе из рутины удаляет её из `routineStack`.

### `enterScope`

При входе в область:

- для `RoutineDeclaration` регистрирует параметры;
- для `ForLoop` регистрирует read-only переменную цикла.

### `leaveScope`

Сейчас не содержит дополнительной логики. Сам `pop` выполняется внутри `AstTraversal` после уведомления observer.

## Отдельный анализатор `return`

Файлы:

```text
checker/return/return.h
checker/return/return.cpp
```

Интерфейс:

```cpp
virtual bool checkReturn(
    const ReturnStatement& statement,
    const RoutineDeclaration* routine,
    ScopeStack& scopeStack
) = 0;
```

Проверки:

1. `return` находится внутри рутины;
2. у рутины есть возвращаемый тип;
3. возвращаемое значение присутствует;
4. тип возвращаемого значения совместим с типом результата рутины.

Поддерживается совместимость:

- одинаковых типов;
- `integer` и `real` с числовым преобразованием;
- `integer` и `boolean` согласно правилам языка;
- номинально одинаковых записей;
- совместимых массивов.

Если выражение уже имеет `errorType`, отдельная производная ошибка не добавляется.

## Порядок обработки `return`

Для программы:

```text
routine add(x : integer) : integer is
    return x + 1
end
```

порядок выглядит так:

```text
enter RoutineDeclaration
    register routine add in outer scope
    push routine scope
    register parameter x
    enter Body
        push body scope
        enter ReturnStatement
            find current routine add
            resolve return type integer
            resolve expression x + 1
            compare types
        leave ReturnStatement
        pop body scope
    leave RoutineDeclaration
    pop routine scope
```

## Стиль кода

Код модуля соответствует стилю проекта:

- используется C++20;
- AST обрабатывается через `NodeType` и `switch`;
- конкретные checker-классы скрыты в `.cpp`;
- публичные интерфейсы находятся в `.h`;
- сервисы создаются фабриками `make...`;
- владение выполняется через `unique_ptr`;
- коллекции используют `vector`, `unordered_map` и `list`;
- заголовки подключаются от корня проекта;
- используется `using namespace std;` на уровне файла;
- методы именуются в стиле `camelCase`;
- сообщения имеют формат `Semantic error (line N): ...`;
- комментарии в исходном коде пишутся на русском языке и описывают ограничения, порядок действий или неочевидные решения.

## Проверка модуля

Сборка:

```bash
cmake --build build
```

Unit-тесты:

```bash
ctest --test-dir build --output-on-failure
```

Golden-тесты:

```bash
./tests/golden.sh check
```

Golden-файлы содержат код завершения и полный stdout compiler. Для семантически некорректных программ ожидается код `1` и сообщение `Semantic error`.

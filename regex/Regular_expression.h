#pragma once
#include "DeterminedFinalAutomat.h"
#include"SintaxTree.h"
namespace rgx {
	using namespace regex;
	class Regular_expression;
//  Простая структура результата анализа строки, содержит вектор строк, куда записываются все подстроки, прошедшие проверку и подходящие под регулярное выражение
// опционально массив из таблиц групп захвата
	struct RgxResult
	{
		std::vector<std::string> goodSubstr;
		std::optional<std::vector<std::unordered_map<std::string, std::string>>> namedGroups;
	};
// функция принимает две строки - анализируемую и регулярное выражение.
// возвращает true если строка подходит целиком, и false если не подходит
// при нарушении синтаксиса выбрасывается sintax_tree_exception
	bool checkString(std::string, std::string);
// выполняет анализ строки как и предыдущая функция, но в объект RgxResult записывает значение строки, если она прошла проверку
// и так же значения из всех групп захвата регулярного выражения (если они есть).
	bool checkString(std::string, std::string, RgxResult&);
// функция выполняет анализ строки по заранее созданному объекту регулярного выражения
	bool checkString(std::string, Regular_expression&)noexcept;
// функция выполняет анализ строки по заранее созданному объекту регулярного выражения и записывает значения всех групп захвата в объект RgxResult
	bool checkString(std::string, Regular_expression&, RgxResult&) noexcept;
// Ищет все непересекающиеся вхождения подстрок строки 1, которые подходят под регулярное выражение строки 2,
// если links = true, в возвращенном объекте помимо подстрок будет массим соответствующих им значений групп зыхвата
	RgxResult findAll(std::string, std::string, bool links = false);
// функиця аналогичная предыдущей, однако анализ выполняется по заранее созданному объекту регулярного выражения
	RgxResult findAll(std::string, Regular_expression&, bool links = false);

// класс регулярного выражения, представляет собой скомпилированный минимальный ДКА (поле класса)
// все функции библиотеки решено было сделать отдельными от экземпляров этого класса, но они являются дружественными для данного класса
// 
// При некорректном синтаксисе выражения будет выброшено исключение sintax_tree_exception, с сообщением какой символ в выражении самый первый нарушает синтаксические правила языка
// 
// Конструктор из string и int - приватный, пользователям библиотеки явно он не доступен, компилирует автомат, но не тратит время на его на минимизацию
// используется в функциях проверки единичной строки по регулярному выражению заданному строкой, после чего автомат будет уничтожен, т.к минимизация автомата не уменьшает скорость
// анализа, а просто уменьшает затраты памяти на его хранение, то если его не нужно хранить, тратить время на минимизацию нет смысла.
// конструктор из string и bool - компилирует минимальный автомат по регулярному выражению, если bool - true, генерируется автомат, соответствующий инверсии заданного языка.
// присутствубт копирующие и перемещающий конструктор
// 
// метод make_language_inversion() возвращает экземпляр нового автомата, соответствующий инверсии языка, по которому построен исходный автомат. (не меняет исходный автомат).
// 
// метод make_language_addition () возвращает экземпляр нового автомата, соответствующий дополненю языка, по которому построен исходный автомат. (не меняет исходный автомат).
// 
// restore_expression() генерирует регулярное выражение, по которому построен данный автомат (явно оно не хранится в автомате, но при необходимости получить выражение стоит использовать этот метод)
// выражение восстанавливается методом восстановления К-путей, поэтому априори получается не минимальным и намного более длинным, чем исходное, однако языки обоих выражений эквивалентны,
// автомат, построенный по восстановленному выражению будет минимальным и идентичным автомату, построенному по исходному выражению.
	class Regular_expression
	{
	private:
		DeterminedFinalAutomat regular_expression_dfa_;
		Regular_expression() {};
		Regular_expression(DeterminedFinalAutomat& at) : regular_expression_dfa_(at) {}
		friend bool checkString(std::string, std::string);
		friend bool checkString(std::string, std::string, RgxResult&);
		friend bool checkString(std::string, Regular_expression&) noexcept;
		friend bool checkString(std::string, Regular_expression&, RgxResult&) noexcept;
		friend RgxResult findAll(std::string, std::string, bool links);
		friend RgxResult findAll(std::string, Regular_expression&, bool links);
		Regular_expression(std::string, int);

	public:
		
		Regular_expression(std::string,bool inverse=false);
		Regular_expression(Regular_expression& obj) :regular_expression_dfa_(obj.regular_expression_dfa_) {}
		Regular_expression(Regular_expression&&);
		~Regular_expression() {};
		std::string restore_expression() const;
		Regular_expression make_language_inversion() const noexcept;
		Regular_expression make_language_addition() const noexcept ;

		Regular_expression& operator=(Regular_expression&);
		Regular_expression& operator=(Regular_expression&&);
	};
	
}
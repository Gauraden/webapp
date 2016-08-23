// Шаблон оформления тестов:
// Символы XXX заменить на название тестируемого модуля, класса и т.п.
// Символы: AAA, BBB, CCC; заменить на названия тестов, отражающих их суть.
// Пример: PopEmptyQueueTest, OpeningFileTest и т.д.
// ------------------------------------------------------------------------------
#define empty() __empty() // cure of boost warning (speaking about shadow variable empty and the same method empty())
#include <boost/test/unit_test.hpp>
// Помощник.
// Данная структура будет передана каждому тесту!
// Конструктор и деструктор обязательны!
struct XXXTestFixture {
    XXXTestFixture() {}
    ~XXXTestFixture() {}
};
// -----------------------------------------------------------------------------
// Инициализация набора тестов
BOOST_FIXTURE_TEST_SUITE(XXXTestSuite, XXXTestFixture)

BOOST_AUTO_TEST_CASE(AAATest) {
  // Пример проверки выражения
  BOOST_CHECK( /* Выражение */ );
  // Пример проверки исключения
  BOOST_CHECK_THROW( /* Выражение */, /* Имя класса исключения */ );
}

BOOST_AUTO_TEST_CASE(BBBTest) {
// TODO: Код теста № 1
}

// Много тестов...

BOOST_AUTO_TEST_CASE(CCCTest) {
// TODO: Код теста № N
}

BOOST_AUTO_TEST_SUITE_END()
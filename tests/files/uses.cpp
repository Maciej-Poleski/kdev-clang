/// "useCount" : 4
int i = 0;

/// "useCount" : 2
int foo()
{
    i = 1;
    return i = i + 1;
}

/// "useCount" : 2
class SomeClass
{
public:
    /// "useCount" : 3
    int i;
    /// "useCount" : 0
    char foo();
};

/// "useCount" : 4
namespace SomeNS
{
    /// "useCount" : 3
    SomeClass someClass;
    /// "useCount" : 2
    int i = foo();
}

int main()
{
    SomeNS::i = i + foo() + SomeNS::someClass.i;
    using namespace SomeNS;
    /// "useCount" : 1
    SomeClass user = someClass;
    user.i = someClass.i = SomeNS::i;
}
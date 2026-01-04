#include "header.h"
#include <sstream>

/**
 * @brief Implementation of ConcreteDerived::implementationMethod
 * 
 * This method provides the specific implementation required by the base class interface.
 * It demonstrates how the derived class can access both its own members and base class members.
 * 
 * @return std::string Implementation-specific result combining derived and base data
 */
std::string ConcreteDerived::implementationMethod() const noexcept {
    std::ostringstream oss;
    oss << "ConcreteDerived implementation - Base data: " << m_baseData
        << ", Name: " << m_name << ", Data: " << m_specificData;
    return oss.str();
}

/**
 * @brief Implementation of ConcreteDerived::stepOne
 * 
 * First step of the algorithm template method. This shows how derived classes
 * can implement specific steps while the base class controls the overall algorithm flow.
 * 
 * @return std::string Result of the first step
 */
std::string ConcreteDerived::stepOne() const noexcept {
    return "Processing initial data: " + m_specificData;
}

/**
 * @brief Implementation of ConcreteDerived::stepTwo
 * 
 * Second step of the algorithm template method.
 * 
 * @return std::string Result of the second step
 */
std::string ConcreteDerived::stepTwo() const noexcept {
    std::ostringstream oss;
    oss << "Validating base data: " << m_baseData 
        << " with name: " << m_name;
    return oss.str();
}

/**
 * @brief Implementation of ConcreteDerived::stepThree
 * 
 * Third step of the algorithm template method.
 * 
 * @return std::string Result of the third step
 */
std::string ConcreteDerived::stepThree() const noexcept {
    return "Finalizing operation for " + m_name;
}

/**
 * @brief Implementation of AnotherDerived::implementationMethod
 * 
 * This shows how different derived classes can provide their own implementations
 * while sharing the same base class interface and functionality.
 * 
 * @return std::string Implementation-specific result
 */
std::string AnotherDerived::implementationMethod() const noexcept {
    std::ostringstream oss;
    oss << "AnotherDerived implementation - Value: " << m_value
        << ", Name: " << m_name;
    return oss.str();
}

/**
 * @brief Example usage function demonstrating CRTP capabilities
 * 
 * This function shows how to use the CRTP classes and highlights the benefits
 * of static polymorphism over dynamic polymorphism.
 */
void demonstrateCRTP() {
    // Create instances of derived classes
    ConcreteDerived concrete("MainInstance", "CriticalData", 100);
    AnotherDerived another("SecondaryInstance", 42);

    std::cout << "=== CRTP Demonstration ===\n\n";

    // Demonstrate interface method (static polymorphism)
    std::cout << "Interface Method Results:\n";
    std::cout << "Concrete: " << concrete.interfaceMethod() << "\n";
    std::cout << "Another: " << another.interfaceMethod() << "\n\n";

    // Demonstrate common functionality with customization
    std::cout << "Common Functionality:\n";
    std::cout << concrete.commonFunctionality("Shared") << "\n";
    std::cout << another.commonFunctionality("Common") << "\n\n";

    // Demonstrate template method pattern
    std::cout << "Algorithm Execution:\n";
    std::cout << concrete.executeAlgorithm() << "\n\n";
    std::cout << another.executeAlgorithm() << "\n\n";

    // Demonstrate that we can use base class pointers/references
    // but with static polymorphism benefits
    const CRTPBaseConcrete& baseRef = concrete;
    std::cout << "Base Reference to Concrete: " 
              << baseRef.interfaceMethod() << "\n";

    const CRTPBaseAnother& anotherBaseRef = another;
    std::cout << "Base Reference to Another: " 
              << anotherBaseRef.interfaceMethod() << "\n\n";

    std::cout << "=== CRTP Benefits ===\n";
    std::cout << "- No virtual function overhead\n";
    std::cout << "- Compile-time polymorphism\n";
    std::cout << "- Better optimization opportunities\n";
    std::cout << "- Type-safe without runtime checks\n";
}

// Optional: If you want to make this a runnable example
#ifdef CRTP_EXAMPLE_MAIN
int main() {
    demonstrateCRTP();
    return 0;
}
#endif
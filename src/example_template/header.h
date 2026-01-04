#pragma once

#include <iostream>
#include <string>
#include <type_traits>

/**
 * @brief CRTP (Curiously Recurring Template Pattern) base class template
 * 
 * This base class provides common functionality that can be shared across
 * multiple derived classes while allowing static polymorphism.
 * 
 * @tparam Derived The derived class type that inherits from this base
 */
template<typename Derived>
class CRTPBase {
public:
    /**
     * @brief Default constructor
     */
    CRTPBase() = default;

    /**
     * @brief Copy constructor
     */
    CRTPBase(const CRTPBase&) = default;

    /**
     * @brief Move constructor
     */
    CRTPBase(CRTPBase&&) = default;

    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~CRTPBase() = default;

    /**
     * @brief Copy assignment operator
     */
    CRTPBase& operator=(const CRTPBase&) = default;

    /**
     * @brief Move assignment operator
     */
    CRTPBase& operator=(CRTPBase&&) = default;

    /**
     * @brief Interface method that uses static polymorphism
     * 
     * This method demonstrates how the base class can call derived class methods
     * at compile time without virtual function overhead.
     * 
     * @return std::string Result from the derived class implementation
     */
    std::string interfaceMethod() const {
        return static_cast<const Derived*>(this)->implementationMethod();
    }

    /**
     * @brief Common functionality that can be used by all derived classes
     * 
     * This method shows how base class can provide shared implementation
     * while still accessing derived class specific functionality.
     * 
     * @param prefix Custom prefix for the message
     * @return std::string Formatted message with derived class information
     */
    std::string commonFunctionality(const std::string& prefix = "Base") const {
        const Derived* derived = static_cast<const Derived*>(this);
        return prefix + ": " + derived->getDerivedName() + " - " + 
               derived->getSpecificData();
    }

    /**
     * @brief Template method pattern implementation using CRTP
     * 
     * This method defines an algorithm skeleton where the base class
     * calls derived class methods to fill in specific steps.
     * 
     * @return std::string Result of the complete algorithm
     */
    std::string executeAlgorithm() const {
        const Derived* derived = static_cast<const Derived*>(this);
        
        // Algorithm steps with hooks for derived classes
        std::string result = "Algorithm start\n";
        result += "Step 1: " + derived->stepOne() + "\n";
        result += "Step 2: " + derived->stepTwo() + "\n";
        result += "Step 3: " + derived->stepThree() + "\n";
        result += "Algorithm complete";
        
        return result;
    }

protected:
    /**
     * @brief Protected constructor to prevent direct instantiation
     */
    CRTPBase(int baseData) : m_baseData(baseData) {}

    /**
     * @brief Base class data that can be accessed by derived classes
     */
    int m_baseData = 0;

    /**
     * @brief Enable safe downcasting using static_assert
     * 
     * This ensures that the Derived class actually inherits from CRTPBase<Derived>
     */
    static void ensureValidInheritance() {
        static_assert(std::is_base_of<CRTPBase<Derived>, Derived>::value,
                     "Derived class must inherit from CRTPBase<Derived>");
    }
};

/**
 * @brief Example derived class using CRTP
 * 
 * This class demonstrates how to properly inherit from the CRTP base class
 * and implement the required interface methods.
 */
class ConcreteDerived : public CRTPBase<ConcreteDerived> {
public:
    /**
     * @brief Constructor with initialization
     * @param name Name of the derived class instance
     * @param data Specific data for this instance
     * @param baseData Data for the base class
     */
    ConcreteDerived(const std::string& name, const std::string& data, int baseData = 42)
        : CRTPBase<ConcreteDerived>(baseData), 
          m_name(name), 
          m_specificData(data) {
        ensureValidInheritance(); // Verify CRTP inheritance at compile time
    }

    /**
     * @brief Implementation of the interface method required by base class
     * @return std::string Implementation-specific result
     */
    std::string implementationMethod() const noexcept;

    /**
     * @brief Get the name of this derived class instance
     * @return std::string Name of the instance
     */
    std::string getDerivedName() const noexcept {
        return m_name;
    }

    /**
     * @brief Get specific data for this derived class
     * @return std::string Specific data
     */
    std::string getSpecificData() const noexcept {
        return m_specificData;
    }

    /**
     * @brief First step of the algorithm
     * @return std::string Result of step one
     */
    std::string stepOne() const noexcept;

    /**
     * @brief Second step of the algorithm
     * @return std::string Result of step two
     */
    std::string stepTwo() const noexcept;

    /**
     * @brief Third step of the algorithm
     * @return std::string Result of step three
     */
    std::string stepThree() const noexcept;

private:
    std::string m_name;
    std::string m_specificData;
};

/**
 * @brief Another example derived class to show CRTP flexibility
 */
class AnotherDerived : public CRTPBase<AnotherDerived> {
public:
    AnotherDerived(const std::string& name, int value)
        : m_name(name), m_value(value) {
        ensureValidInheritance();
    }

    std::string implementationMethod() const noexcept;
    std::string getDerivedName() const noexcept { return m_name; }
    std::string getSpecificData() const noexcept { return std::to_string(m_value); }
    std::string stepOne() const noexcept { return "Another step 1"; }
    std::string stepTwo() const noexcept { return "Another step 2"; }
    std::string stepThree() const noexcept { return "Another step 3"; }

private:
    std::string m_name;
    int m_value;
};

// Type aliases for convenience
using CRTPBaseConcrete = CRTPBase<ConcreteDerived>;
using CRTPBaseAnother = CRTPBase<AnotherDerived>;
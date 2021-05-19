#include "pch.h"
#include <cold/com/refcounted.h>
#include <cold/com/ianything.h>
#include <cold/com/comclass.h>
#include <cold/utils/functor.h>
#include <cold/diagnostics/logging.h>



using namespace cold;

using namespace testing;

namespace {


struct NonVirtualBase1
{
};


struct Base1 : NonVirtualBase1
{
	DECLARE_CLASS_BASE(NonVirtualBase1)
};

struct Base2 : NonVirtualBase1
{
	DECLARE_CLASS_BASE(NonVirtualBase1)

	void print2() {
		std::cout << "Base2\n";
	}
};

struct Base3 : IAnything
{
};


struct MyClass1 : Base1, Base2, Base3, cold::IRefCounted
{
	COMCLASS_(Base1, Base2, Base3)


public:

	std::string value = "uno";


	~MyClass1() {
		std::cout << "destructed:" << value << std::endl;
	}
};


struct INTERFACE_API IBaseRC1 : virtual IRefCounted
{
};

struct INTERFACE_API  IBaseRC2 : virtual IRefCounted
{
};


struct INTERFACE_API Interface1 : IBaseRC1, IBaseRC2
{
	DECLARE_CLASS_BASE(IBaseRC1, IBaseRC2)

	virtual void f1() = 0;
};

struct INTERFACE_API Interface2 : IRefCounted
{
	virtual void f2() const = 0;
};


struct INTERFACE_API Interface3
{
	virtual ~Interface3() = default;
};


struct NotImplemented : IRefCounted
{};





class MyCoClass final
	: public Interface1
	, public Interface2
	, public Interface3
{
	COMCLASS_(Interface1, Interface2, Interface3)

public:

	using Callback = Functor<void()>;


	MyCoClass() = default;

	MyCoClass(Callback::UniquePtr callback)
		: m_callback(std::move(callback))
	{}

	~MyCoClass() {
		if (m_callback) {
			m_callback->invoke();
		}
	}

private:

	virtual void f1() override
	{}

	virtual void f2() const override
	{}


	Callback::UniquePtr m_callback;
};

enum class CoClassAllocationType
{
	DefaultAllocator,
	CustomAllocator,
	InplaceMemory
};

template<typename T>
decltype(auto) operator << (std::basic_ostream<T>& stream, CoClassAllocationType type) {
	if (type == CoClassAllocationType::CustomAllocator) {
		stream << "Custom allocator";
	}
	else if (type == CoClassAllocationType::DefaultAllocator) {
		stream << "Default allocator";
	}
	else {
		stream << "Inplace mem";
	}

	return (stream);
}


class TestAllocator final : public Allocator
{
	COMCLASS_(Allocator)
public:
	void* realloc(void* prevPtr, size_t size, std::optional<size_t>) override {
		return _aligned_realloc(prevPtr, size, sizeof(void*));
	}

	
	void free(void* ptr, std::optional<size_t> = std::nullopt) override {
		_aligned_free(ptr);
	}

};



class Test_CoClass : public ::testing::TestWithParam<CoClassAllocationType>
{
protected:

	ComPtr<IRefCounted> createTestInstance() {

		const CoClassAllocationType allocationType = GetParam();

		if (allocationType == CoClassAllocationType::DefaultAllocator) {
			return com::createInstance<MyCoClass, IRefCounted>();
		}
		
		if (allocationType == CoClassAllocationType::InplaceMemory) {
			return com::createInstanceInplace<MyCoClass, IRefCounted>(m_inplaceStorage);
		}

		DEBUG_CHECK(allocationType == CoClassAllocationType::CustomAllocator)
		m_customAllocator = com::createInstanceInplace<TestAllocator, Allocator>(m_allocatorInplaceStorage);

		return com::createInstanceWithAllocator<MyCoClass, IRefCounted>(m_customAllocator);
	}

private:
	
	alignas(sizeof(uint64_t)) char m_inplaceStorage[com::InstanceStorageSize<MyCoClass>];
	alignas(sizeof(uint64_t)) char m_allocatorInplaceStorage[com::InstanceStorageSize<TestAllocator>];

	ComPtr<Allocator> m_customAllocator;
};



//-----------------------------------------------------------------------------
TEST_P(Test_CoClass, IsRefCounted)
{
	auto itf = createTestInstance();

	ASSERT_TRUE(itf);
	ASSERT_TRUE(itf->is<IRefCounted>());
}


TEST_P(Test_CoClass, CastToRefCounted)
{
	auto itf = createTestInstance();
	auto anything = itf->as<IAnything*>();
	ASSERT_THAT(anything, NotNull());

	auto refCounted = anything->as<IRefCounted*>();
	ASSERT_THAT(refCounted , NotNull());
}


TEST_P(Test_CoClass, IsAnything)
{
	auto itf = createTestInstance();

	ASSERT_TRUE(itf->is<IAnything>());
	ASSERT_TRUE(itf->as<IAnything*>() != nullptr);
}


TEST_P(Test_CoClass, InterfaceAccess)
{
	auto itf = createTestInstance();

	ASSERT_TRUE(itf->is<Interface1>());
	ASSERT_TRUE(itf->is<Interface2>());
	ASSERT_TRUE(itf->is<Interface3>());
	ASSERT_TRUE(itf->is<IBaseRC1>());
	ASSERT_TRUE(itf->is<IBaseRC2>());


	ASSERT_THAT(itf->as<Interface1*>(), NotNull());
	ASSERT_THAT(itf->as<Interface2*>(), NotNull());
	ASSERT_THAT(itf->as<Interface3*>(), NotNull());
	ASSERT_THAT(itf->as<IBaseRC1*>(), NotNull());
	ASSERT_THAT(itf->as<IBaseRC2*>(), NotNull());

	ASSERT_FALSE(itf->is<std::string>());
	ASSERT_THAT(itf->as<std::string*>(), IsNull());
}


TEST_P(Test_CoClass, WeakReferenceNotNull)
{
	auto itf = createTestInstance();
	IWeakReference* const weakRef = itf->getWeakRef();
	ASSERT_THAT(weakRef, NotNull());
	weakRef->release();
}


TEST_P(Test_CoClass, WeakReferenceNotDeadWhileInstanceAlive)
{
	auto itf = createTestInstance();
	IWeakReference* const weakRef = itf->getWeakRef();
	ASSERT_FALSE(weakRef->isDead());
	weakRef->release();
}


TEST_P(Test_CoClass, WeakReferenceIsDeadAfterInstanceReleased)
{
	IWeakReference* const weakRef = createTestInstance()->getWeakRef();
	ASSERT_TRUE(weakRef->isDead());
	weakRef->release();
}


TEST_P(Test_CoClass, WeakReferenceAcquire)
{
	auto itf = createTestInstance();
	IWeakReference* const weakRef = itf->getWeakRef();
	IRefCounted* const instance = weakRef->acquire();
	ASSERT_THAT(instance, NotNull());
}


TEST_P(Test_CoClass, WeakReferenceAcquireNull)
{
	IWeakReference* const weakRef = createTestInstance()->getWeakRef();
	IRefCounted* const instance = weakRef->acquire();
	ASSERT_THAT(instance, IsNull());
}


INSTANTIATE_TEST_SUITE_P(
	Default,
	Test_CoClass,
	testing::ValuesIn({CoClassAllocationType::DefaultAllocator, CoClassAllocationType::CustomAllocator, CoClassAllocationType::InplaceMemory})
);


TEST(Test_Com, InstanceStorageSize)
{
	constexpr size_t TypeStorageSize = com::InstanceStorageSize<MyCoClass>;
	ASSERT_THAT(TypeStorageSize, Gt(sizeof(MyCoClass)));
}



}


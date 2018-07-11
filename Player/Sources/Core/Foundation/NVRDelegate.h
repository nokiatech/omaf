
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#pragma once

#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFCompiler.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

//
// Lightweight delegate implementation without virtual functions and dynamic memory allocation.
//
// Syntax to declare delegates:
//
//      Declares delegate with no arguments and void_t return value:
//
//          Delegate<void_t (void_t)> delegate;
//
//      Declares delegate with int32_t argument and void_t return value:
//
//          Delegate<void_t (int32_t)> delegate;
//
//      Declares delegate with int32_t, float32_t arguments and int32_t return value:
//
//          Delegate<int32_t (int32_t, float32_t)> delegate;
//
//
// Syntax to bind delegates to functions:
//
//      Bind free / static member function:
//
//          delegate.bind<&FreeFunction>();
//
//      Bind class member function:
//
//          Class c;
//          delegate.bind<Class, &Class::MemberFunction>(&c);
//
//      Bind const class member function:
//
//          Class c;
//          delegate.bind<Class, &Class::ConstMemberFunction>(&c);
//
//
// Invoke delegates:
//
//      Invoke delegate function:
//
//          delegate.invoke();
//          delegate();
//
//
// Compare delegates:
//
//      Delegates can be compared with standard == and != operators.
//
//
// Note: Extreme caution should be considered with static global function bindings
//       e.g. MSVC threats static global functions with same signature in different translation units as same.
//       This causes delegate bound to static global function invoke a call to wrong implementation eventually.
//       As a workaround to this issue it is recommended to use nameless namespace instead static for global functions
//       as it will generate unique signatures for each function in different translation units.
//

template <typename T>
class Delegate {};

template <typename ReturnValue>
class Delegate<ReturnValue ()>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)()>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)() const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)()>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance)
    {
        return (Function)();
    }
    
    template <class Class, ReturnValue (Class::*Function)()>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance)
    {
        return ((Class*)(instance.ptr)->*Function)();
    }
    
    template <class Class, ReturnValue (Class::*Function)() const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance)
    {
        return ((const Class*)(instance.constPtr)->*Function)();
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)()>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)()>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)() const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke() const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance);
    }
     
    ReturnValue operator () () const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0>
class Delegate<ReturnValue (ARG0)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0)
    {
        return (Function)(arg0);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0);
    }
    
    ReturnValue operator () (ARG0 arg0) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1>
class Delegate<ReturnValue (ARG0, ARG1)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1)
    {
        return (Function)(arg0, arg1);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2>
class Delegate<ReturnValue (ARG0, ARG1, ARG2)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        return (Function)(arg0, arg1, arg2);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        return (Function)(arg0, arg1, arg2, arg3);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3, ARG4)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3, arg4);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3, arg4);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }

    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
public:
    
    Delegate()
    {
    }
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

template <typename ReturnValue, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class Delegate<ReturnValue (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
{
private:
    
    struct InstancePtr
    {
        InstancePtr()
        : ptr(0)
        {
        }
        
        union
        {
            void_t* ptr;
            const void_t* constPtr;
        };
        
        bool_t operator == (const InstancePtr& other) const
        {
            return (ptr == other.ptr);
        }
        
        bool_t operator != (const InstancePtr& other) const
        {
            return (ptr != other.ptr);
        }
    };
    
    typedef ReturnValue (*InternalFunction)(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9);
    
    struct FunctionPtr
    {
        InstancePtr instance;
        InternalFunction function;
        
        bool_t operator == (const FunctionPtr& other) const
        {
            return (instance == other.instance) && (function == other.function);
        }
        
        bool_t operator != (const FunctionPtr& other) const
        {
            return (instance != other.instance) || (function != other.function);
        }
    };
    
    FunctionPtr mFunctionPtr;
    
public:
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
    struct Wrapper
    {
        Wrapper(Class* instance)
        : instance(instance)
        {
        }
        
        Class* instance;
    };
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) const>
    struct ConstWrapper
    {
        ConstWrapper(const Class* instance)
        : instance(instance)
        {
        }
        
        const Class* instance;
    };
    
public:
    
    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
    static OMAF_INLINE ReturnValue callFunctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
    static OMAF_INLINE ReturnValue callMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9)
    {
        return ((Class*)(instance.ptr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) const>
    static OMAF_INLINE ReturnValue callConstMemberFuctionPtr(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9)
    {
        return ((const Class*)(instance.constPtr)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    
public:
    
    Delegate()
    {
    }

    template <ReturnValue (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
    void_t bind()
    {
        mFunctionPtr.instance.ptr = OMAF_NULL;
        mFunctionPtr.function = &callFunctionPtr<Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
    void_t bind(Wrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.ptr = wrapper.instance;
        mFunctionPtr.function = &callMemberFuctionPtr<Class, Function>;
    }
    
    template <class Class, ReturnValue (Class::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) const>
    void_t bind(ConstWrapper<Class, Function> wrapper)
    {
        mFunctionPtr.instance.constPtr = wrapper.instance;
        mFunctionPtr.function = &callConstMemberFuctionPtr<Class, Function>;
    }
    
    ReturnValue invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    
    ReturnValue operator () (ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9) const
    {
        OMAF_ASSERT(mFunctionPtr.function != OMAF_NULL, "Cannot invoke, bind function first");
        
        return mFunctionPtr.function(mFunctionPtr.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    
    bool_t operator == (const Delegate& other) const
    {
        return (mFunctionPtr == other.mFunctionPtr);
    }
    
    bool_t operator != (const Delegate& other) const
    {
        return (mFunctionPtr != other.mFunctionPtr);
    }
};

OMAF_NS_END

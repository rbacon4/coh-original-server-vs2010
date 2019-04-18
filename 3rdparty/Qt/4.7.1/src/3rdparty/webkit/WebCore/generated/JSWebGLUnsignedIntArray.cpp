/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(3D_CANVAS)

#include "JSWebGLUnsignedIntArray.h"

#include "WebGLUnsignedIntArray.h"
#include <runtime/Error.h>
#include <runtime/JSNumberCell.h>
#include <runtime/PropertyNameArray.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSWebGLUnsignedIntArray);

/* Hash table for prototype */

static const HashTableValue JSWebGLUnsignedIntArrayPrototypeTableValues[3] =
{
    { "get", DontDelete|Function, (intptr_t)static_cast<NativeFunction>(jsWebGLUnsignedIntArrayPrototypeFunctionGet), (intptr_t)1 },
    { "set", DontDelete|Function, (intptr_t)static_cast<NativeFunction>(jsWebGLUnsignedIntArrayPrototypeFunctionSet), (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static JSC_CONST_HASHTABLE HashTable JSWebGLUnsignedIntArrayPrototypeTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 3, JSWebGLUnsignedIntArrayPrototypeTableValues, 0 };
#else
    { 4, 3, JSWebGLUnsignedIntArrayPrototypeTableValues, 0 };
#endif

const ClassInfo JSWebGLUnsignedIntArrayPrototype::s_info = { "WebGLUnsignedIntArrayPrototype", 0, &JSWebGLUnsignedIntArrayPrototypeTable, 0 };

JSObject* JSWebGLUnsignedIntArrayPrototype::self(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSWebGLUnsignedIntArray>(exec, globalObject);
}

bool JSWebGLUnsignedIntArrayPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, &JSWebGLUnsignedIntArrayPrototypeTable, this, propertyName, slot);
}

bool JSWebGLUnsignedIntArrayPrototype::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<JSObject>(exec, &JSWebGLUnsignedIntArrayPrototypeTable, this, propertyName, descriptor);
}

const ClassInfo JSWebGLUnsignedIntArray::s_info = { "WebGLUnsignedIntArray", &JSWebGLArray::s_info, 0, 0 };

JSWebGLUnsignedIntArray::JSWebGLUnsignedIntArray(NonNullPassRefPtr<Structure> structure, JSDOMGlobalObject* globalObject, PassRefPtr<WebGLUnsignedIntArray> impl)
    : JSWebGLArray(structure, globalObject, impl)
{
}

JSObject* JSWebGLUnsignedIntArray::createPrototype(ExecState* exec, JSGlobalObject* globalObject)
{
    return new (exec) JSWebGLUnsignedIntArrayPrototype(JSWebGLUnsignedIntArrayPrototype::createStructure(JSWebGLArrayPrototype::self(exec, globalObject)));
}

bool JSWebGLUnsignedIntArray::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    bool ok;
    unsigned index = propertyName.toUInt32(&ok, false);
    if (ok && index < static_cast<WebGLUnsignedIntArray*>(impl())->length()) {
        slot.setValue(getByIndex(exec, index));
        return true;
    }
    return Base::getOwnPropertySlot(exec, propertyName, slot);
}

bool JSWebGLUnsignedIntArray::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    bool ok;
    unsigned index = propertyName.toUInt32(&ok, false);
    if (ok && index < static_cast<WebGLUnsignedIntArray*>(impl())->length()) {
        descriptor.setDescriptor(getByIndex(exec, index), DontDelete);
        return true;
    }
    return Base::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

bool JSWebGLUnsignedIntArray::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (propertyName < static_cast<WebGLUnsignedIntArray*>(impl())->length()) {
        slot.setValue(getByIndex(exec, propertyName));
        return true;
    }
    return getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

void JSWebGLUnsignedIntArray::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    bool ok;
    unsigned index = propertyName.toUInt32(&ok, false);
    if (ok) {
        indexSetter(exec, index, value);
        return;
    }
    Base::put(exec, propertyName, value, slot);
}

void JSWebGLUnsignedIntArray::put(ExecState* exec, unsigned propertyName, JSValue value)
{
    indexSetter(exec, propertyName, value);
    return;
}

void JSWebGLUnsignedIntArray::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    for (unsigned i = 0; i < static_cast<WebGLUnsignedIntArray*>(impl())->length(); ++i)
        propertyNames.add(Identifier::from(exec, i));
     Base::getOwnPropertyNames(exec, propertyNames, mode);
}

JSValue JSC_HOST_CALL jsWebGLUnsignedIntArrayPrototypeFunctionGet(ExecState* exec, JSObject*, JSValue thisValue, const ArgList& args)
{
    UNUSED_PARAM(args);
    if (!thisValue.inherits(&JSWebGLUnsignedIntArray::s_info))
        return throwError(exec, TypeError);
    JSWebGLUnsignedIntArray* castedThisObj = static_cast<JSWebGLUnsignedIntArray*>(asObject(thisValue));
    WebGLUnsignedIntArray* imp = static_cast<WebGLUnsignedIntArray*>(castedThisObj->impl());
    unsigned index = args.at(0).toInt32(exec);


    JSC::JSValue result = jsNumber(exec, imp->get(index));
    return result;
}

JSValue JSC_HOST_CALL jsWebGLUnsignedIntArrayPrototypeFunctionSet(ExecState* exec, JSObject*, JSValue thisValue, const ArgList& args)
{
    UNUSED_PARAM(args);
    if (!thisValue.inherits(&JSWebGLUnsignedIntArray::s_info))
        return throwError(exec, TypeError);
    JSWebGLUnsignedIntArray* castedThisObj = static_cast<JSWebGLUnsignedIntArray*>(asObject(thisValue));
    return castedThisObj->set(exec, args);
}


JSValue JSWebGLUnsignedIntArray::getByIndex(ExecState* exec, unsigned index)
{
    return jsNumber(exec, static_cast<WebGLUnsignedIntArray*>(impl())->item(index));
}
WebGLUnsignedIntArray* toWebGLUnsignedIntArray(JSC::JSValue value)
{
    return value.inherits(&JSWebGLUnsignedIntArray::s_info) ? static_cast<JSWebGLUnsignedIntArray*>(asObject(value))->impl() : 0;
}

}

#endif // ENABLE(3D_CANVAS)
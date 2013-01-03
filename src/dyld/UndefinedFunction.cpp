/*
This file is part of Darling.

Copyright (C) 2012 Lubos Dolezel

Darling is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Darling is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Darling.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "UndefinedFunction.h"
#include <unistd.h>
#include <stdexcept>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/IRBuilder.h>
#include <llvm/Constant.h>
#include <llvm/Type.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/TargetSelect.h>
#include <stdio.h>

UndefMgr::UndefMgr()
{
	std::string es;
	llvm::InitializeNativeTarget();
	m_module = new llvm::Module("UndefMgr", llvm::getGlobalContext());

	llvm::EngineBuilder eb = llvm::EngineBuilder(m_module);
	eb.setEngineKind(llvm::EngineKind::JIT);
	eb.setErrorStr(&es);

	m_engine = eb.create();

	llvm::Type* params1_p[2];

	params1_p[0] = llvm::PointerType::getInt32PtrTy(llvm::getGlobalContext());
	params1_p[1] = llvm::PointerType::getInt8PtrTy(llvm::getGlobalContext());

	llvm::ArrayRef<llvm::Type*> params1(params1_p,2);
	llvm::FunctionType* functionType = llvm::FunctionType::get(/*Result=*/ llvm::IntegerType::get(llvm::getGlobalContext(), 32),
		  /*Params=*/ params1, /*isVarArg=*/ true);

	m_llvm_fprintf = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("fprintf", functionType));
}

UndefMgr::~UndefMgr()
{
	delete m_engine;
}

void* UndefMgr::generateNew(const char* name)
{
	auto itPrev = m_generated.find(name);
	if (itPrev != m_generated.end())
		return itPrev->second;

	auto itNew = m_generated.insert(m_generated.end(), std::pair<std::string, void*>(name, nullptr));
	
	llvm::Function* uf = llvm::cast<llvm::Function>( m_module->getOrInsertFunction(name, llvm::Type::getVoidTy(llvm::getGlobalContext()), NULL));

	llvm::BasicBlock* entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", uf);
	llvm::IRBuilder<> b(entry);
	llvm::Value* f = llvm::cast<llvm::Value>(
		llvm::ConstantExpr::getIntToPtr(
			llvm::ConstantInt::get(
				llvm::PointerType::getInt32PtrTy(llvm::getGlobalContext()),
				uint64_t(stderr)
			),
			llvm::Type::getInt32PtrTy(llvm::getGlobalContext())
		)
	);
   
	llvm::Value* pp0 = b.CreateGlobalStringPtr("Undefined function called: %s\n");
	llvm::Value* pp1 = b.CreateGlobalStringPtr(itNew->first.c_str());

	b.CreateCall3(m_llvm_fprintf, f, pp0, pp1, "fprintf");
	b.CreateRet(nullptr);

	llvm::verifyModule(*m_module);

	return itNew->second = m_engine->getPointerToFunction(uf);
}


#ifdef TEST
int main()
{
	UndefMgr* mgr = new UndefMgr;
	int (*func)() = (int(*)()) mgr->generateNew("TestFunction");
	int (*func2)() = (int(*)()) mgr->generateNew("TestFunction2");
	
	func();
	func2();
	
	delete mgr;
	return 0;
}
#endif

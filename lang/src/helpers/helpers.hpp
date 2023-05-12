#pragma once

#include <optional>
#include <cstdint>

#include "llvm_headers.hpp"
#include <external/external.hpp>
#include <scope/scope.hpp>

namespace Interp {

bool CheckIfCellIsLambda(ObjectPtr maybe_lambda_cell);

std::string ObjectToString(ObjectPtr value);
std::vector<ObjectPtr> ListToVector(ObjectPtr init);
std::string ListToString(ObjectPtr init);

} // namespace Interp

namespace Codegen {

/*
    Singleton class to share
    application resources between files
*/
struct Context {
public:
    static Context& Get();

    Context(const Context&) = delete;
    Context& operator=(Context&) = delete;
    ~Context();

    std::optional<llvm::LLVMContext> llvm_context;
    std::optional<llvm::Module> llvm_module;
    std::optional<llvm::IRBuilder<>> builder;
    llvm::StructType* object_type;

    std::stack<llvm::Value*> last_stack_saves;

private:
    Context();

    void SetExternalFunctions();
    void SetExternalFunction(std::string name, llvm::Type* return_value_type, const std::vector<llvm::Type*>& argument_types);
};

// COPYING
llvm::Value* CreateValueCopy(llvm::Value* object_value, llvm::BasicBlock* object_value_branch);
llvm::Value* CreateValueCopy(const std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>& object_value_and_branch_vector);

// STACK
void CreateStackSave();
void CreateStackRestore();

// CELL
llvm::Value* CreateStoreNewCell();
void CreateStoreCellFirst(llvm::Value* object_value, llvm::Value* first_value);
void CreateStoreCellSecond(llvm::Value* object_value, llvm::Value* second_value);

// BUILD NEW FROM CPP TYPE
llvm::Value* CreateStoreNewNumber(int64_t number);
llvm::Value* CreateStoreNewSymbol(std::string symbol);
llvm::Value* CreateStoreNewBoolean(bool boolean);

// BUILD NEW FROM OBJECT TYPE
llvm::Value* CreateStoreNewNumber(llvm::Value* number_value);
llvm::Value* CreateStoreNewSymbol(llvm::Value* symbol_value);
llvm::Value* CreateStoreNewBoolean(llvm::Value* boolean_value);

// STORE FROM OBJECT TYPE
void CreateStoreNumber(llvm::Value* object_value, llvm::Value* number_value);
// void CreateStoreSymbol(llvm::Value* object_value, llvm::Value* symbol_value);
void CreateStoreBoolean(llvm::Value* object_value, llvm::Value* boolean_value);

// LOAD CPP TYPE FROM OBJECT TYPE
llvm::Value* CreateLoadNumber(llvm::Value* object_value);
// llvm::Value* CreateLoadSymbol(llvm::Value* object_value);
llvm::Value* CreateLoadBoolean(llvm::Value* object_value);

// CHECKS
void CreateObjectTypeCheck(llvm::Value* object_value, ObjectType type);
void CreateObjectTypeCheck(llvm::Value* object_value, ObjectType type, llvm::BasicBlock* true_branch, llvm::BasicBlock* false_branch);
void CreateIsIntegerCheck(llvm::Value* number_value);
llvm::Value* CreateIsZeroThenOneCheck(llvm::Value* number_value);

// CHECK FOR SPECIFIC VALUE
using PairValueBB = std::pair<llvm::Value*, llvm::BasicBlock*>;

template <bool boolean_smth>
std::vector<PairValueBB> CreateIsBooleanSmthThenBranch(llvm::Value* object_value, llvm::BasicBlock* continue_branch, llvm::BasicBlock* end_branch) {
    auto& context = Context::Get();
    auto old_branch = context.builder->GetInsertBlock();

    auto& llvm_context = context.llvm_context.value();
    llvm::Function* current_function = context.builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* is_boolean_branch = llvm::BasicBlock::Create(llvm_context, "is_boolean_branch", current_function);
    llvm::BasicBlock* is_boolean_smth_branch = llvm::BasicBlock::Create(llvm_context, "is_boolean_smth_branch", current_function);

    // std::vector<llvm::Value*> object_value_type_field_indices {
    //     context.builder->getInt32(0), // because there is no array, so just the object itself
    //     context.builder->getInt32(FieldType::FIELD_TYPE)
    // };
    // llvm::Value* object_value_type_field = context.builder->CreateGEP(context.object_type, object_value, object_value_type_field_indices);
    // llvm::Value* object_value_type = context.builder->CreateLoad(context.builder->getInt64Ty(), object_value_type_field);
    // llvm::Value* is_type_boolean = context.builder->CreateICmpEQ(object_value_type, context.builder->getInt64(ObjectType::TYPE_BOOLEAN), "is_boolean_check");
    // context.builder->CreateCondBr(is_type_boolean, is_boolean_branch, continue_branch);
    CreateObjectTypeCheck(object_value, ObjectType::TYPE_BOOLEAN, is_boolean_branch, continue_branch);
    
    context.builder->SetInsertPoint(is_boolean_branch);
    llvm::Value* boolean_value = CreateLoadBoolean(object_value);
    llvm::Value* is_boolean_smth = context.builder->CreateICmpEQ(boolean_value, context.builder->getInt1(boolean_smth), "is_boolean_smth_check");
    llvm::Value* object_value_copy_in_is_boolean_branch = CreateValueCopy(object_value, old_branch);
    context.builder->CreateCondBr(is_boolean_smth, is_boolean_smth_branch, continue_branch);

    context.builder->SetInsertPoint(is_boolean_smth_branch);
    llvm::Value* object_value_copy_in_is_boolean_smth_branch = CreateValueCopy(object_value_copy_in_is_boolean_branch, is_boolean_branch);
    context.builder->CreateBr(end_branch);

    context.builder->SetInsertPoint(continue_branch);
    llvm::Value* object_value_copy_in_continue_branch = CreateValueCopy({
        {object_value, old_branch},
        {object_value_copy_in_is_boolean_branch, is_boolean_branch}
    });

    std::vector<PairValueBB> ans {
        {object_value_copy_in_is_boolean_smth_branch, is_boolean_smth_branch},
        {object_value_copy_in_continue_branch, continue_branch}
    };
    return ans;
}

} // namespace Codegen
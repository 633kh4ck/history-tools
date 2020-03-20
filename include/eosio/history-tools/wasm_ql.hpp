#pragma once
#include <eosio/history-tools/callbacks/action.hpp>
#include <eosio/history-tools/callbacks/console.hpp>
#include <eosio/history-tools/callbacks/kv.hpp>
#include <eosio/ship_protocol.hpp>
#include <eosio/vm/backend.hpp>

namespace eosio { namespace wasm_ql {

class backend_cache;

struct shared_state {
   uint32_t                            max_console_size = {};
   uint32_t                            wasm_cache_size  = {};
   uint64_t                            max_exec_time_ms = {};
   std::string                         contract_dir     = {};
   std::unique_ptr<backend_cache>      backend_cache    = {};
   std::shared_ptr<chain_kv::database> db;

   shared_state(std::shared_ptr<chain_kv::database> db);
   ~shared_state();
};

struct thread_state : eosio::history_tools::action_state, eosio::history_tools::console_state {
   std::shared_ptr<const shared_state> shared = {};
   eosio::vm::wasm_allocator           wa     = {};
};

class thread_state_cache {
 private:
   std::mutex                                   mutex;
   std::shared_ptr<const wasm_ql::shared_state> shared_state;
   std::vector<std::unique_ptr<thread_state>>   states;

 public:
   thread_state_cache(const std::shared_ptr<const wasm_ql::shared_state>& shared_state) : shared_state(shared_state) {}

   std::unique_ptr<thread_state> get_state() {
      std::lock_guard<std::mutex> lock{ mutex };
      if (states.empty()) {
         auto result    = std::make_unique<thread_state>();
         result->shared = shared_state;
         return result;
      }
      auto result = std::move(states.back());
      states.pop_back();
      return result;
   }

   void store_state(std::unique_ptr<thread_state> state) {
      std::lock_guard<std::mutex> lock{ mutex };
      states.push_back(std::move(state));
   }
};

void register_callbacks();

const std::vector<char>& query_get_info(wasm_ql::thread_state&   thread_state,
                                        const std::vector<char>& contract_kv_prefix);
const std::vector<char>& query_get_block(wasm_ql::thread_state&   thread_state,
                                         const std::vector<char>& contract_kv_prefix, std::string_view body);
const std::vector<char>& query_get_abi(wasm_ql::thread_state& thread_state, const std::vector<char>& contract_kv_prefix,
                                       std::string_view body);
const std::vector<char>& query_get_required_keys(wasm_ql::thread_state& thread_state, std::string_view body);
const std::vector<char>& query_send_transaction(wasm_ql::thread_state&   thread_state,
                                                const std::vector<char>& contract_kv_prefix, std::string_view body);
ship_protocol::transaction_trace_v0 query_send_transaction(wasm_ql::thread_state&                   thread_state,
                                                           const std::vector<char>&                 contract_kv_prefix,
                                                           const ship_protocol::packed_transaction& trx,
                                                           const rocksdb::Snapshot*                 snapshot,
                                                           std::vector<std::vector<char>>&          memory);

}} // namespace eosio::wasm_ql

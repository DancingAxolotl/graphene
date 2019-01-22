/*
 * Copyright (c) 2018 oxarbitrage and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

      /**
       * Temporally save music_contract transactions until funds are released or operation expired.
       */
      class music_contract_object : public graphene::db::abstract_object<music_contract_object> {
         public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id  = impl_music_contract_object_type;

            uint32_t                contract_id=10;
            account_id_type         producer;
            account_id_type         musician;
            account_id_type         agent;
            asset                   amount;
            time_point_sec          ratification_deadline;
            time_point_sec          contract_expiration;
            asset                   pending_fee;
            bool                    signed = false;
            bool                    agent_approved = false;
            bool                    disputed = false;

            bool is_approved()const { return signed && agent_approved; }
      };

      struct by_producer_id;
      struct by_ratification_deadline;
      struct by_expiration;
      typedef multi_index_container<
         music_contract_object,
         indexed_by<
            ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,

            ordered_non_unique< tag< by_expiration >, member< music_contract_object, time_point_sec, &music_contract_object::contract_expiration > >,

            ordered_unique< tag< by_producer_id >,
               composite_key< music_contract_object,
                  member< music_contract_object, account_id_type,  &music_contract_object::producer >,
                  member< music_contract_object, uint32_t, &music_contract_object::contract_id >
               >
            >,
            ordered_unique< tag< by_ratification_deadline >,
               composite_key< music_contract_object,
                  const_mem_fun< music_contract_object, bool, &music_contract_object::is_approved >,
                  member< music_contract_object, time_point_sec, &music_contract_object::ratification_deadline >,
                  member< music_contract_object, uint32_t, &music_contract_object::contract_id >
               >,
               composite_key_compare< std::less< bool >, std::less< time_point_sec >, std::less< uint32_t > >
            >
         >

      > music_contract_object_index_type;

      typedef generic_index< music_contract_object, music_contract_object_index_type > music_contract_index;

   } }

FC_REFLECT_DERIVED( graphene::chain::music_contract_object, (graphene::db::object),
                    (contract_id)(producer)(musician)(agent)(ratification_deadline)(contract_expiration)(pending_fee)(amount)(disputed)(musician_approved)(agent_approved) );

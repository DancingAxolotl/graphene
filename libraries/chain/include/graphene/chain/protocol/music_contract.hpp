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
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

      /**
       *  The purpose of this operation is to enable someone to send money contingently to
       *  another individual. The funds leave the *producer* account and go into a temporary balance
       *  where they are held until *producer* releases it to *musician* or *musician* refunds it to *producer*.
       *
       *  In the event of a dispute the *agent* can divide the funds between the musician/producer account.
       *  Disputes can be raised any time before or on the dispute deadline time, after the contract
       *  has been approved by all parties.
       *
       *  This operation only creates a proposed contract transfer. Both the *agent* and *musician* must
       *  agree to the terms of the arrangement by approving the contract.
       *
       *  The contract agent is paid the fee on approval of all parties. It is up to the agent
       *  to determine the fee.
       *
       *  Transactions are uniquely identified by 'producer' and 'music_contract_id', the 'music_contract_id' is defined
       *  by the sender.
       */
      struct music_contract_transfer_operation : public base_operation {

         struct fee_parameters_type {
            uint64_t fee            = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         };
         asset                   fee;

         account_id_type         producer;
         account_id_type         musician;
         asset                   amount;

         uint32_t                music_contract_id=0;
         account_id_type         agent;
         asset                   agent_fee;
         string                  json_meta;
         time_point_sec          ratification_deadline;
         time_point_sec          music_contract_expiration;

         void validate()const;
         void get_required_active_authorities( flat_set<account_id_type>& a )const{ a.insert(producer); }
         account_id_type fee_payer()const { return producer; }
      };

      /**
       *  The agent and musician accounts must approve an music_contract transaction for it to be valid on
       *  the blockchain. Once a part approves the music_contract, the cannot revoke their approval.
       *  Subsequent music_contract approve operations, regardless of the approval, will be rejected.
       */
      struct music_contract_approve_operation : public base_operation
      {
         struct fee_parameters_type {
            uint64_t fee            = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         };
         asset                   fee;

         account_id_type         producer;
         account_id_type         musician;
         account_id_type         agent;
         account_id_type         who; // Either musician or agent
         uint32_t                music_contract_id;
         bool                    approve;

         void validate()const;
         void get_required_active_authorities( flat_set<account_id_type>& a )const{ a.insert(who); }
         account_id_type fee_payer()const { return who; }
      };

      /**
       *  If either the sender or receiver of an music_contract payment has an issue, they can
       *  raise it for dispute. Once a payment is in dispute, the agent has authority over
       *  who gets what.
       */
      struct music_contract_dispute_operation : public base_operation {

         struct fee_parameters_type {
            uint64_t fee            = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         };
         asset                   fee;

         account_id_type         producer;
         account_id_type         musician;
         account_id_type         agent;
         uint32_t                music_contract_id;
         account_id_type         who;

         void validate()const;
         void get_required_active_authorities( flat_set<account_id_type>& a )const{ a.insert(who); }
         account_id_type fee_payer()const { return who; }
      };

      /**
       *  This operation can be used by anyone associated with the music_contract transfer to
       *  release funds if they have permission.
       *
       *  The permission scheme is as follows:
       *  If there is no dispute and music_contract has not expired, either party can release funds to the other.
       *  If music_contract expires and there is no dispute, either party can release funds to either party.
       *  If there is a dispute regardless of expiration, the agent can release funds to either party
       *     following whichever agreement was in place between the parties.
       */
      struct music_contract_release_operation : public base_operation {

         struct fee_parameters_type {
            uint64_t fee            = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         };
         asset                   fee;

         account_id_type         producer;
         account_id_type         musician;
         account_id_type         agent;
         account_id_type         who;
         account_id_type         receiver;
         uint32_t                music_contract_id;
         asset                   amount;

         void validate()const;
         void get_required_active_authorities( flat_set<account_id_type>& a )const{ a.insert(who); }
         account_id_type fee_payer()const { return who; }
      };

   } }

FC_REFLECT( graphene::chain::music_contract_transfer_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::music_contract_approve_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::music_contract_dispute_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::music_contract_release_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::music_contract_transfer_operation, (fee)(producer)(musician)(agent)(amount)(music_contract_id)(agent_fee)(json_meta)(ratification_deadline)(music_contract_expiration) );
FC_REFLECT( graphene::chain::music_contract_approve_operation, (fee)(producer)(musician)(agent)(who)(music_contract_id)(approve) );
FC_REFLECT( graphene::chain::music_contract_dispute_operation, (fee)(producer)(musician)(agent)(who)(music_contract_id) );
FC_REFLECT( graphene::chain::music_contract_release_operation, (fee)(producer)(musician)(agent)(who)(receiver)(music_contract_id)(amount) );

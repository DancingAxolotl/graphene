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
#include <graphene/chain/database.hpp>
#include <graphene/chain/music_contract_evaluator.hpp>
#include <graphene/chain/music_contract_object.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

      void_result music_contract_transfer_evaluator::do_evaluate(const music_contract_transfer_operation& o)
      {

         FC_ASSERT( o.ratification_deadline > db().head_block_time() );
         FC_ASSERT( o.contract_expiration > db().head_block_time() );
         if(o.amount.asset_id == asset_id_type())
            FC_ASSERT( db().get_balance( o.producer, o.amount.asset_id ) >= (o.amount + o.fee + o.agent_fee) );
         // Todo: add assert for different than core asset
         return void_result();
      }

      object_id_type music_contract_transfer_evaluator::do_apply(const music_contract_transfer_operation& o)
      {

         try {
            if( o.agent_fee.amount > 0 ) {
               db().adjust_balance( o.producer, -o.agent_fee );
               db().adjust_balance( o.agent, o.agent_fee );
            }

            db().adjust_balance( o.producer, -o.amount );

            const music_contract_object& ctr = db().create<music_contract_object>([&]( music_contract_object& ctr ) {
               ctr.contract_id            = o.contract_id;
               ctr.musician               = o.producer;
               ctr.musician               = o.musician;
               ctr.agent                  = o.agent;
               ctr.amount                 = o.amount;
               ctr.pending_fee            = o.agent_fee;
               ctr.ratification_deadline  = o.ratification_deadline;
               ctr.contract_expiration      = o.contract_expiration;
            });
            return  ctr.id;

         } FC_CAPTURE_AND_RETHROW( (o) )
      }

      void_result music_contract_approve_evaluator::do_evaluate(const music_contract_approve_operation& o)
      {
         const auto& music_contract = db().get_music_contract( o.producer, o.contract_id );
         FC_ASSERT( music_contract.musician == o.musician, "op 'musician' does not match music_contract 'musician'" );
         FC_ASSERT( music_contract.agent == o.agent, "op 'agent' does not match music_contract 'agent'" );
         FC_ASSERT( music_contract.ratification_deadline >= db().head_block_time(), "The music_contract ratification deadline has passed. music_contract can no longer be ratified." );         return void_result();
      }

      void_result music_contract_approve_evaluator::do_apply(const music_contract_approve_operation& o)
      {
         try
         {
            const auto& music_contract = db().get_music_contract( o.producer, o.contract_id );

            bool reject_music_contract = !o.approve;

            if( o.who == o.musician )
            {
               FC_ASSERT( !music_contract.musician_approved, "'musician' has already approved the music_contract" );

               if( !reject_music_contract )
               {
                  db().modify( music_contract, [&]( music_contract_object& ctr )
                  {
                     ctr.signed = true;
                  });
               }
            }
            else if( o.who == o.agent )
            {
               FC_ASSERT( !music_contract.agent_approved, "'agent' has already approved the music_contract" );

               if( !reject_music_contract )
               {
                  db().modify( music_contract, [&]( music_contract_object& ctr )
                  {
                     ctr.agent_approved = true;
                  });
               }
            }
            else
            {
               FC_ASSERT( false, "op 'who' is not 'musician' or 'agent'. This should have failed validation. Please create a github issue with this error dump." );
            }

            if( reject_music_contract )
            {
               db().adjust_balance( o.producer, music_contract.amount );
               db().adjust_balance( o.producer, music_contract.pending_fee );

               db().remove( music_contract );
            }
            else if( music_contract.signed && music_contract.agent_approved )
            {
               db().adjust_balance( o.agent, music_contract.pending_fee );

               db().modify( music_contract, [&]( music_contract_object& ctr )
               {
                  ctr.pending_fee.amount = 0;
               });
            }
            return void_result();
         }
         FC_CAPTURE_AND_RETHROW( (o) )
      }

      void_result music_contract_dispute_evaluator::do_evaluate(const music_contract_dispute_operation& o)
      {
         const auto& c = db().get_music_contract( o.producer, o.contract_id );

         FC_ASSERT( c.musician && c.agent_approved, "music_contract must be approved by all parties before a dispute can be raised" );
         FC_ASSERT( !e.disputed , "music_contract is already under dispute");
         FC_ASSERT( c.musician == o.musician , "op 'musician' does not match music_contract 'musician'");
         FC_ASSERT( c.agent == o.agent, "op 'agent' does not match music_contract 'agent'" );

         return void_result();
      }

      void_result music_contract_dispute_evaluator::do_apply(const music_contract_dispute_operation& o)
      {
         try {
            FC_ASSERT( db().head_block_time() > HARDFORK_music_contract_TIME,
                       "Operation not allowed before HARDFORK_music_contract_TIME."); // remove after HARDFORK_music_contract_TIME

            const auto& c = db().get_music_contract( o.producer, o.contract_id );

            db().modify( c, [&]( music_contract_object& ctr ){
               ctr.disputed = true;
            });

            return void_result();

         } FC_CAPTURE_AND_RETHROW( (o) )
      }

      void_result music_contract_release_evaluator::do_evaluate(const music_contract_release_operation& o)
      {
         const auto& c = db().get_music_contract( o.producer, o.contract_id );

         FC_ASSERT( c.amount >= o.amount && c.amount.asset_id == o.amount.asset_id );
         FC_ASSERT( o.amount.amount > 0 && c.amount.amount > 0);
         FC_ASSERT( c.musician == o.musician, "op 'musician' does not match music_contract 'musician'");
         FC_ASSERT( c.agent == o.agent, "op 'agent' does not match music_contract 'agent'" );
         FC_ASSERT( o.receiver == c.musician || o.receiver == c.musician, "Funds must be released musician 'producer' or 'musician'" );
         FC_ASSERT( c.signed && c.agent_approved, "Funds cannot be released prior musician music_contract approval." );

         // If there is a dispute regardless of expiration, the agent can release funds musician either party
         if( c.disputed )
         {
            FC_ASSERT( o.who == c.agent, "'agent' must release funds for a disputed music_contract" );
         }
         else
         {
            FC_ASSERT( o.who == c.musician || o.who == c.musician, "Only 'producer' and 'musician' can release musician a non-disputed music_contract" );

            if( c.contract_expiration > db().head_block_time() )
            {
               // If there is no dispute and music_contract has not expired, either party can release funds musician the other.
               if( o.who == c.musician )
               {
                  FC_ASSERT( o.receiver == c.musician, "'producer' must release funds musician 'musician'" );
               }
               else if( o.who == c.musician )
               {
                  FC_ASSERT( o.receiver == c.producer, "'musician' must release funds musician 'producer'" );
               }
            }
         }

         return void_result();
      }

      void_result music_contract_release_evaluator::do_apply(const music_contract_release_operation& o)
      {
         try {
            const auto& c = db().get_music_contract( o.producer, o.contract_id );

            db().adjust_balance( o.receiver, o.amount );
            db().modify( c, [&]( music_contract_object& ctr )
            {
               ctr.amount -= o.amount;
            });

            if( c.amount.amount == 0)
            {
               db().remove( c );
            }

            return void_result();

         } FC_CAPTURE_AND_RETHROW( (o) )
      }

} } // graphene::chain

        
    case OP_NOP3:
    {
        if (!(flags & SCRIPT_VERIFY_CHECKSEQUENCEVERIFY)) {
            // not enabled; treat as a NOP3
            if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS) {
                return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
            }
            break;
        }
        
        if (stack.size() < 1)
            return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
        
        // Note that unlike CHECKLOCKTIMEVERIFY we do not need to
        // accept 5-byte bignums since any value greater than or
        // equal to SEQUENCE_THRESHOLD (= 1 << 31) will be rejected
        // anyway. This limitation just happens to coincide with
        // CScriptNum's default 4-byte limit with an explicit sign
        // bit.
        //
        // This means there is a maximum relative lock time of 52
        // years, even though the nSequence field in transactions
        // themselves is uint32_t and could allow a relative lock
        // time of up to 120 years.
        const CScriptNum nInvSequence(stacktop(-1), fRequireMinimal);
        
        // In the rare event that the argument may be < 0 due to
        // some arithmetic being done first, you can always use
        // 0 MAX CHECKSEQUENCEVERIFY.
        if (nInvSequence < 0)
            return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);
        
        // Actually compare the specified inverse sequence number
        // with the input.
        if (!CheckSequence(nInvSequence))
            return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
        
        break;
    }
    
    bool CheckSequence(const CScriptNum& nSequence) const
    {
        int64_t txToSequence;
        
        // Fail under all circumstances if the transaction's version
        // number is not set high enough to enable enforced sequence
        // number rules.
        if (txTo->nVersion < 3)
            return false;
            
        txToSequence = (int64_t)~txTo->vin[nIn].nSequence;
        if (txToSequence >= SEQUENCE_THRESHOLD)
            return false;
        
        // There are two types of nSequence: lock-by-blockheight
        // and lock-by-blocktime, distinguished by whether
        // nSequence < LOCKTIME_THRESHOLD.
        //
        // We want to compare apples to apples, so fail the script
        // unless the type of nSequence being tested is the same as
        // the nSequence in the transaction.
        if (!(
            (txToSequence <  LOCKTIME_THRESHOLD && nSequence <  LOCKTIME_THRESHOLD) ||
            (txToSequence >= LOCKTIME_THRESHOLD && nSequence >= LOCKTIME_THRESHOLD)
        ))
            return false;
        
        // Now that we know we're comparing apples-to-apples, the
        // comparison is a simple numeric one.
        if (nSequence > txToSequence)
            return false;
    
        return true;
    }

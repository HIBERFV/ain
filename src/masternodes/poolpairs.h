// Copyright (c) 2019 The DeFi Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEFI_MASTERNODES_POOLPAIRS_H
#define DEFI_MASTERNODES_POOLPAIRS_H

#include <flushablestorage.h>

#include <amount.h>
#include <arith_uint256.h>
#include <masternodes/res.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <masternodes/balances.h>

struct CPoolPairMessage {
    DCT_ID idTokenA, idTokenB;
    CAmount commission;
    bool status = true;
    std::string pairSymbol;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(VARINT(idTokenA.v));
        READWRITE(VARINT(idTokenB.v));
        READWRITE(commission);
        READWRITE(status);
        READWRITE(pairSymbol);
    }
};

class CPoolPair
{
public:
    static const CAmount MINIMUM_LIQUIDITY = 1000;
    static const CAmount PRECISION = COIN; // or just PRECISION_BITS for "<<" and ">>"

    DCT_ID tokenA, tokenB;
    CAmount reserveA, reserveB, totalLiquidity;

    arith_uint256 priceACumulativeLast, priceBCumulativeLast; // not sure about 'arith', at least sqrt() undefined
    arith_uint256 kLast;
    uint32_t lastPoolEventHeight;

    CAmount commissionPct;   // comission %% for traders
    CAmount rewardPct;       // pool yield farming reward %%
    CScript ownerFeeAddress;

    uint256 creationTx;
    uint32_t creationHeight;

    CPoolPairMessage poolPairMsg;

    ResVal<CPoolPair> Create(CPoolPairMessage const & msg);     // or smth else
//    ResVal<CTokenAmount> AddLiquidity(CTokenAmount const & amountA, CTokenAmount amountB, CScript const & shareAddress);
    // or:
//    ResVal<CTokenAmount> AddLiquidity(CLiquidityMessage const & msg);

    // ????
//    ResVal<???> RemoveLiquidity(???);
//    ResVal<???> Swap(???);

    // 'amountA' && 'amountB' should be normalized (correspond) to actual 'tokenA' and 'tokenB' ids in the pair!!
    // otherwise, 'AddLiquidity' should be () external to 'CPairPool' (i.e. CPoolPairView::AddLiquidity(TAmount a,b etc) with internal lookup of pool by TAmount a,b)
    Res AddLiquidity(CAmount const & amountA, CAmount amountB, CScript const & shareAddress, std::function<Res(CScript to, CAmount liqAmount)> onMint, uint32_t height) {

        mintFee(onMint); // if fact, this is delayed calc (will be 0 on the first pass) // deps: reserveA(R), reserveB(R), kLast, totalLiquidity(RW)

        CAmount liquidity;

//        CAmount _totalLiquidity = totalLiquidity;
        if (totalLiquidity == 0) {
            liquidity = ((arith_uint256(amountA) * arith_uint256(amountB)).sqrt() - MINIMUM_LIQUIDITY).GetLow64();
//           _mint(address(0), MINIMUM_LIQUIDITY); // permanently lock the first MINIMUM_LIQUIDITY tokens
            totalLiquidity = MINIMUM_LIQUIDITY;
        } else {
//            liquidity = Math.min(amount0.mul(_totalLiquidity) / _reserve0, amount1.mul(_totalLiquidity) / _reserve1);
            CAmount liqA = (arith_uint256(amountA) * totalLiquidity / reserveA).GetLow64();
            CAmount liqB = (arith_uint256(amountB) * totalLiquidity / reserveB).GetLow64();
            liquidity = std::min(liqA, liqB);
        }
//        require(liquidity > 0, 'UniswapV2: INSUFFICIENT_LIQUIDITY_MINTED');
        onMint(shareAddress, liquidity); // deps: totalLiquidity(RW)

        update(reserveA+amountA /*balance0*/, reserveB+amountB /*balance1*//*, _reserve0, _reserve1*/, height); // deps: prices, reserves, kLast
        if (!ownerFeeAddress.empty()) kLast = arith_uint256(reserveA) * arith_uint256(reserveB); // reserve0 and reserve1 are up-to-date
        return Res::Ok();
    }

    // usage:
    //    Res onMint(CScript const address, CAmount amount) {
    //        totalLiquidity += amount;
    //        Account(address)->Add(lpTokenID, amount);
    //        WriteBy<ByShare>(lpTokenID, address);
    //    }

    Res RemoveLiquidity(CScript const & address, CAmount const & liqAmount, std::function<Res(CScript to, CAmount amountA, CAmount amountB)> onMint, uint32_t height) {

        CAmount resAmountA, resAmountB;

        resAmountA = liqAmount * reserveA / totalLiquidity;
        resAmountB = liqAmount * reserveB / totalLiquidity;

        onMint(address, resAmountA, resAmountB);

        update(reserveA - resAmountA, reserveB - resAmountB, height); // deps: prices, reserves, kLast

        return Res::Ok();
    }

    Res Swap(CAmount amount0Out, CAmount amount1Out, CScript to) {
//        require(amount0Out > 0 || amount1Out > 0, 'UniswapV2: INSUFFICIENT_OUTPUT_AMOUNT');
//        (uint112 _reserve0, uint112 _reserve1,) = getReserves(); // gas savings
//        require(amount0Out < _reserve0 && amount1Out < _reserve1, 'UniswapV2: INSUFFICIENT_LIQUIDITY');

//        uint balance0;
//        uint balance1;
//        { // scope for _token{0,1}, avoids stack too deep errors
//        address _token0 = token0;
//        address _token1 = token1;
//        require(to != _token0 && to != _token1, 'UniswapV2: INVALID_TO');
//        if (amount0Out > 0) _safeTransfer(_token0, to, amount0Out); // optimistically transfer tokens
//        if (amount1Out > 0) _safeTransfer(_token1, to, amount1Out); // optimistically transfer tokens
//        if (data.length > 0) IUniswapV2Callee(to).uniswapV2Call(msg.sender, amount0Out, amount1Out, data);
//        balance0 = IERC20(_token0).balanceOf(address(this));
//        balance1 = IERC20(_token1).balanceOf(address(this));
//        }
//        uint amount0In = balance0 > _reserve0 - amount0Out ? balance0 - (_reserve0 - amount0Out) : 0;
//        uint amount1In = balance1 > _reserve1 - amount1Out ? balance1 - (_reserve1 - amount1Out) : 0;
//        require(amount0In > 0 || amount1In > 0, 'UniswapV2: INSUFFICIENT_INPUT_AMOUNT');
//        { // scope for reserve{0,1}Adjusted, avoids stack too deep errors
//        uint balance0Adjusted = balance0.mul(1000).sub(amount0In.mul(3));
//        uint balance1Adjusted = balance1.mul(1000).sub(amount1In.mul(3));
//        require(balance0Adjusted.mul(balance1Adjusted) >= uint(_reserve0).mul(_reserve1).mul(1000**2), 'UniswapV2: K');
//        }

//        _update(balance0, balance1, _reserve0, _reserve1);
//        emit Swap(msg.sender, amount0In, amount1In, amount0Out, amount1Out, to);


        return Res::Ok();
    }

private:
    /// @todo review all 'Res' uses
    void mintFee(std::function<Res(CScript to, CAmount liqAmount)> onMint) {
//        auto _kLast = kLast;
        if (!ownerFeeAddress.empty()) {
            if (kLast != 0) {
                auto rootK = (arith_uint256(reserveA) * arith_uint256(reserveB)).sqrt();
                auto rootKLast = kLast.sqrt();
                if (rootK > rootKLast) {
                    auto numerator = arith_uint256(totalLiquidity) * (rootK - rootKLast);
                    auto denominator = rootK * 5 + rootKLast;
                    auto liquidity = (numerator / denominator).GetLow64(); // fee ~= 1/6 of delta K (square median) - all math got from uniswap smart contract
                    if (liquidity > 0) onMint(ownerFeeAddress, liquidity);
                }
            }
        } else /*if (_kLast != 0) */{
            kLast = 0;
        }
    }


    Res update(CAmount balanceA, CAmount balanceB, /*CAmount _reserveA, CAmount _reserveB, - prev values*/ uint32_t height) {
//        require(balance0 <= uint112(-1) && balance1 <= uint112(-1), 'UniswapV2: OVERFLOW');

//        uint32 blockTimestamp = uint32(block.timestamp % 2**32);
        if (height > lastPoolEventHeight && reserveA != 0 && reserveB != 0) {
            const uint32_t timeElapsed = height - lastPoolEventHeight;
            // * never overflows, and + overflow is desired
            priceACumulativeLast += arith_uint256(reserveB) * timeElapsed * PRECISION / (reserveA); // multiplied by COIN for precision (!!!)
            priceBCumulativeLast += arith_uint256(reserveA) * timeElapsed * PRECISION / (reserveB);
//            priceBCumulativeLast += uint(UQ112x112.encode(_reserve0).uqdiv(_reserve1)) * timeElapsed;
        }
        reserveA = balanceA;
        reserveB = balanceB;
        lastPoolEventHeight = height;
//        emit Sync(reserve0, reserve1);
        return Res::Ok();
    }

public:

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {

    }
};

class CPoolPairView : public virtual CStorageView
{
public:
    Res SetPoolPair(DCT_ID & poolId, CPoolPair const & pool);
    Res DeletePoolPair(DCT_ID const & poolId);

    boost::optional<CPoolPair> GetPoolPair(DCT_ID const & poolId) const;
//    boost::optional<std::pair<DCT_ID, CPoolPair> > GetPoolPairGuessId(const std::string & str) const; // optional
    boost::optional<std::pair<DCT_ID, CPoolPair> > GetPoolPair(DCT_ID const & tokenA, DCT_ID const & tokenB) const;

    void ForEachPoolPair(std::function<bool(DCT_ID const & id, CPoolPair const & pool)> callback, DCT_ID const & start = DCT_ID{0});
    void ForEachShare(std::function<bool(DCT_ID const & id, CScript const & provider)> callback, DCT_ID const & start = DCT_ID{0});
//    void ForEachShare(std::function<bool(DCT_ID const & id, CScript const & provider, CAmount amount)> callback, DCT_ID const & start = DCT_ID{0}); // optional, with lookup into accounts

//    Res AddLiquidity(CTokenAmount const & amountA, CTokenAmount amountB, CScript const & shareAddress);

    // tags
    struct ByID { static const unsigned char prefix; }; // lsTokenID -> СPoolPair
    struct ByPair { static const unsigned char prefix; }; // tokenA+tokenB -> lsTokenID
    struct ByShare { static const unsigned char prefix; }; // lsTokenID+accountID -> {}
};

struct CLiquidityMessage {
    std::map<CScript, CBalances> from; // from -> balances
    CScript shareAddress;

    std::string ToString() const {
        if (from.empty()) {
            return "empty transfer";
        }
        std::string result;
        for (const auto& kv : from) {
            result += "(" + kv.first.GetHex() + "->" + kv.second.ToString() + ")";
        }
        result += " to " + shareAddress.GetHex();
        return result;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(from);
        READWRITE(shareAddress);
    }
};

struct CRemoveLiquidityMessage {
    std::map<CScript, CBalances> from; // from -> balances

    std::string ToString() const {
        if (from.empty()) {
            return "empty transfer";
        }
        std::string result;
        for (const auto& kv : from) {
            result += "(" + kv.first.GetHex() + "->" + kv.second.ToString() + ")";
        }
        return result;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(from);
    }
};


#endif // DEFI_MASTERNODES_POOLPAIRS_H

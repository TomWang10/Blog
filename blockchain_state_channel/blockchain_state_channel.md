# 比特币闪电网络

## 什么是支付隧道(Payment channels)
支付隧道是用于在无信任环境中进行链下交易比特币的一种机制。交易双方通过广播并锁定一笔由双方共同签名的链上交易。隧道(channel)只是一种虚拟的比喻，一个隧道由一个在链上的(on-chain)的*funding transaction*、一系列的链下(off-chain)的*commitment transaction*和最后一笔链上交易组成。因为绝大部分的交易都是在链下发生，所以就没有了像比特币一样的交易速度的限制，极大的提高了交易的速度。  

## 最简单的单向支付隧道
比如一个视频点播app，用户(记作A)每播放1秒钟视频需要支付0.1个比特币给app开发者(记作B)，在app界面上提供了一个基于用户和app开发者双重签名的地址，用户向该地址发送了360个比特币作为该支付隧道的*funding transaction*，这笔双重交易的输出初始状态是t0(A:360, B:0)，随着视频的播放，双方每隔一秒钟就会有一笔新的*commitment transaction*生产，t1(A:359.9, B:0.1), t2(A:359.8, B:0.2)···，这些承诺交易都是不广播到链上的，最终用户点击了停止播放按钮，最后一笔交易的状态可能是tn(A:100，B:260)，最后一笔交易就广播到了链上。整个交易完成，并且只在链上产生了两笔交易。  

这个简单的单向支付隧道有两个明显的漏洞:
* A发送第一笔资金交易后，B直接消失，那A永远也拿不回自己钱了。
* A可以拿被B签名过后的任何一笔*commitment transaction*进行广播，从而减少自己费用。

这两个问题可以通过加一个*timelock*来解决

## 加了时间锁的单向支付隧道
在A发起*funding transaction*之前，A先让B签名一份有*timelock*的*refund transaction*，交易的输出的高度为当前的区块高度+3000。然后才开始接下来的*funding transaction*流程，如果B消失了，A可以通过*refund transaction*等待(3000个块)拿回自己的钱。接下来的每一次*commitment transaction*都带着时间锁，从*refund transaction*设定的区块高度递减，比如T+3000, T+2999, T+2000, T+1500。这样每一次签署最新的*commitment transaction*就相当于撤销了之前的交易(因为时间离T较近，可以优先广播出去)。这也就解决了上面说的A可以拿之前的一笔*commitment transaction*进行广播，然而B看到A广播了之前的区块，就可以拿时间较新的交易进行广播，从而让A的广播的交易失效。  

 简单的加了*time lock*的单向支付隧道虽然解决了上面两个问题，但还有几个问题没有解决:
 * 如果B故意消失的话，A只能等到3000个区块后才能拿回自己的钱
 * 因为添加了*time lock*，隧道的生命周期有了限制。
 * 因为每一笔交易都是通过递减时间锁进行的，所以*commitment transaction*笔数有了限制。

为了解决这三个问题，接下来要引入叫*Asymmetric Revocable Commitments*

## Asymmetric Revocable Commitments
和前面一样，我们先在链上广播一笔由双方签名的*funding transacion*,但接下来的*commitment transaction*和前面的不同，A和B双方各自生成一个*commitment transaction*，像下面这样:  

A的*commitment transaction*:  

```shell
Input: 2-of-2 funding output, signed by B

Output 0 <5 bitcoin>:
    <B's Public Key> CHECKSIG

Output 1 <5 bitcoin>:
    <1000 blocks>
    CHECKSEQUENCEVERIFY
    DROP
    <A's Public Key> CHECKSIG
```

B的*commitment transaction*:  

```shell
Input: 2-of-2 funding output, signed by A

Output 0 <5 bitcoin>:
    <A's Public Key> CHECKSIG

Output 1 <5 bitcoin>:
    <1000 blocks>
    CHECKSEQUENCEVERIFY
    DROP
    <B's Public Key> CHECKSIG
```

因为双方都签名的交易可以马上拿回自己的币，而对方得等待1000个区块后才能拿回自己的币。所以双方都很愿意对交易进行签名。接下来要加入一个新的元素：*Revocation Public Key*每次发起*commitment transaction*前双方都会各自独立生成该密码的一半，把这个Key用在上面的第二个output上，变成这样:  

A的*commitment transaction*:  

```shell
Input: 2-of-2 funding output, signed by B

Output 0 <5 bitcoin>:
    <B's Public Key> CHECKSIG

Output 1 <5 bitcoin>:
IF
    # Revocation pernalty output
    <Revocation Public Key>
ELSE
    <1000 blocks>
    CHECKSEQUENCEVERIFY
    DROP
    <A's Public Key> CHECKSIG
ENDIF
CHECKSIG
```

B的*commitment transaction*:  

```shell
Input: 2-of-2 funding output, signed by A

Output 0 <5 bitcoin>:
    <A's Public Key> CHECKSIG

Output 1 <5 bitcoin>:
IF
    # Revocation pernalty output
    <Revocation Public Key>
ELSE
    <1000 blocks>
    CHECKSEQUENCEVERIFY
    DROP
    <B's Public Key> CHECKSIG
ENDIF
CHECKSIG
```

有了*Revocation key*，在每次提议*commitment transaction*前都会交换前面一个*commitment transaction*的*Revocation public key*，如果一方发现对方广播了前面的一笔*commitment transaction*就可以使用完整的*Revocation Key*拿到全部的币。通过使用了相对的*time lock*就可以极大提高单个隧道里面的*commitment transaction*了

## Hash Time Lock Contracts(HTLC)
HTLC是一种智能合约，它想到达到的效果就是由交易的接受人提供一个Hash过的密钥R作为output的的条件之一，如果谁能出示密钥R，就能获得这个交易的output的币。类似这样的输出:  

```shell
IF
    # Payment if you have the secret R
    HASH160 <H> EQUALVERIFY
ELSE
    # Refund after timeout.
    <locktime> CHECKLOCKTIMEVERIFY DROP
    <Payer Public Key> CHECKSIG
ENDIF
```

## 路由的支付隧道(Lighting Network)
在HTLC的机制基础上我们可以实现一个简单的闪电网络，在双方没有建立支付隧道的情况下完成一笔交易。例子如下图:  
![routed_payment_channel](routed_payment_channel.png)
Alice想向Eric转账1BTC, 因为Alice并没有直接跟Eric建立隧道，他只知道Bob，所以他向Bob发起了一笔*commitment transaction*，一共1.003BTC，output指向持有Eric提供的密钥R的收款人。因为Bob并没有密钥R，所以他转而向跟他建立隧道的Carol发起一笔*commitment transaction*，一共1.002BTC,这样如果Carol能向他提供密钥R的话，他转可以获得1.003-1.002=0.001个BTC, 因为Carol也没有密钥R，所以他向Diana发起另外一笔1.001BTC的*commitment transaction*，期望Diana能给他密钥R，最后到达了Eirc,他提供了密钥R，因此所有人都获得了对应的*commitment transaciont*，交易完成。这就是最简单的闪电网络。

#include "Bank.h"

#include "UserWithSigningAuthority.h"

void Bank::issueTo(PublicKey receiver) {
  auto transfer = Transfer::Issuance(*this, receiver);
  m_keys.sign(transfer);
  auto newCoin = Coin::CreateByIssuing(transfer);
  m_coins.insert(newCoin);
}

std::set<Coin> Bank::coinsOwnedBy(PublicKey owner) const {
  auto coins = std::set<Coin>{};
  for (auto &coin : m_coins) {
    if (coin.currentOwner() == owner) coins.insert(coin);
  }
  return coins;
}

void Bank::observe(Coin toBeUpdated) {
  if (!isCoinValid(toBeUpdated)) return;

  m_coins.erase(toBeUpdated);
  m_coins.insert(toBeUpdated);
}

bool Bank::isCoinValid(Coin coin) const {
  if (coin.isEmpty()) return true;

  return coinWasIssuedByThisBank(coin) &&
         coin.allTransactionsHaveValidSignatures() &&
         coin.eachSpenderWasTheOwner();
}

bool Bank::coinWasIssuedByThisBank(Coin coin) const {
  const auto bankPublicKey = m_keys.getWeakVersion();
  return coin.issuer() == bankPublicKey;
}
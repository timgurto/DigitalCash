#include "Bank.h"

Transaction Bank::issue(Currency amount, PublicKey recipient) {
  const auto txID = generateTxID();
  const auto output = TxOutput{txID, 0, amount, recipient};
  const auto issuance = Transaction{txID, {}, {output}};

  registerTransaction(issuance);
  m_transactions[txID] = issuance;
  m_unspentOutputs.insert(output.id);
  return issuance;
}

Currency Bank::checkBalance(PublicKey account) const {
  auto balance = Currency{0};

  for (const auto& outputID : m_unspentOutputs) {
    const auto& utxo = lookupOutput(outputID);
    if (utxo.recipient == account) balance += utxo.amount;
  }

  return balance;
}

void Bank::handleTransaction(const Transaction& tx) {
  if (!transactionIsValid(tx)) return;

  takeCoinsFromInputs(tx.inputs);
  giveCoinsToOutputs(tx.outputs);

  registerTransaction(tx);
}

void Bank::registerTransaction(const Transaction& tx)

{
  m_transactions[tx.id] = tx;
}

bool Bank::transactionIsValid(const Transaction& tx) const {
  return inputsAreUnspent(tx.inputs) && inputsMatchOutputs(tx) &&
         inputsAreSigned(tx);
}

bool Bank::inputsAreUnspent(const Transaction::Inputs& inputs) const {
  auto toBeSpentInThisTransaction = std::set<TxOutputID>{};
  for (const auto& input : inputs) {
    const auto alreadyIncludedInThisTransaction =
        toBeSpentInThisTransaction.count(input.previousOutput) == 1;
    if (alreadyIncludedInThisTransaction) return false;

    const auto wasSpentBeforeThisTransaction =
        m_unspentOutputs.count(input.previousOutput) == 0;
    if (wasSpentBeforeThisTransaction) return false;
    toBeSpentInThisTransaction.insert(input.previousOutput);
  }

  return true;
}

bool Bank::inputsMatchOutputs(const Transaction& tx) const {
  return sum(tx.inputs) == sum(tx.outputs);
}

bool Bank::inputsAreSigned(const Transaction& tx) const {
  for (auto index = 0; index != tx.inputs.size(); ++index) {
    if (!tx.inputs[index].signature.exists()) return false;
    const auto sender = correspondingUTXO(tx.inputs[index]).recipient;
    const auto message = tx.getMessageForInput(index);
    if (!sender.verifySignatureForMessage(tx.inputs[index].signature, message))
      return false;
  }

  return true;
}

Currency Bank::sum(const Transaction::Inputs& inputs) const {
  auto total = Currency{0};
  for (const auto& input : inputs) {
    const auto inputAmount = correspondingUTXO(input).amount;
    total += inputAmount;
  }
  return total;
}

Currency Bank::sum(const Transaction::Outputs& outputs) {
  auto total = Currency{0};
  for (const auto& output : outputs) total += output.amount;
  return total;
}

void Bank::takeCoinsFromInputs(const Transaction::Inputs& inputs) {
  for (const auto& input : inputs) {
    m_unspentOutputs.erase(input.previousOutput);
  }
}

const TxOutput& Bank::correspondingUTXO(const TxInput& input) const {
  const auto outputID = input.previousOutput;
  return lookupOutput(outputID);
}

void Bank::giveCoinsToOutputs(const Transaction::Outputs& outputs) {
  for (const auto& output : outputs) giveOutputToItsRecipient(output);
}

void Bank::giveOutputToItsRecipient(const TxOutput& output) {
  m_unspentOutputs.insert(output.id);
}

const TxOutput& Bank::lookupOutput(const TxOutputID& outputID) const {
  const auto parentTx = m_transactions.find(outputID.transaction);
  return parentTx->second.outputs[outputID.outputIndex];
}

//
// Copyright 2015 - 2017 (C). Alex Robenko. All rights reserved.
//

// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include <iterator>
#include <type_traits>
#include "comms/field/IntValue.h"
#include "ProtocolLayerBase.h"
#include "details/ChecksumLayerOptionsParser.h"

namespace comms
{

namespace protocol
{
/// @brief Protocol layer that is responsible to calculate checksum on the
///     data written by all the wrapped internal layers and prepend it before
///     the written data. When reading, this layer is responsible to verify
///     the checksum reported at the beginning of the read data.
/// @details This protocol layer handles checksum value that usually precedes the
///     data. Some protocols use checksum value that follows the data. In this
///     case use @ref ChecksumLayer instead.
/// @tparam TField Type of the field that is used as to represent checksum value.
/// @tparam TCalc The checksum calculater class that is used to calculate
///     the checksum value on the provided buffer. It must have the operator()
///     defined with the following signature:
///     @code
///     template <typename TIter>
///     ResultType operator()(TIter& iter, std::size_t len) const;
///     @endcode
///     It is up to the checksum calculator to choose the "ResultType" it
///     returns. The falue is going to be casted to Field::ValueType before
///     assigning it as a value of the check field being read and/or written.
/// @tparam TNextLayer Next transport layer in protocol stack.
/// @tparam TOptions Extending functionality options. Supported options are:
///     @li comms::option::ChecksumPrefixLayerVerifyBeforeRead - By default, the
///         @b ChecksumPrefixLayer will invoke @b read operation of inner (wrapped) layers
///         and only if it is successful, it will calculate and verify the
///         checksum value. Usage of comms::option::ChecksumPrefixLayerVerifyBeforeRead
///         modifies the default behaviour by forcing the checksum verification
///         prior to invocation of @b read operation in the wrapped layer(s).
/// @headerfile comms/protocol/ChecksumPrefixLayer.h
template <typename TField, typename TCalc, typename TNextLayer, typename... TOptions>
class ChecksumPrefixLayer : public ProtocolLayerBase<TField, TNextLayer>
{
    using Base = ProtocolLayerBase<TField, TNextLayer>;
public:
    /// @brief Parsed options
    using ParsedOptions = details::ChecksumLayerOptionsParser<TOptions...>;

    /// @brief Type of the field object used to read/write checksum value.
    using Field = typename Base::Field;

    /// @brief Default constructor.
    ChecksumPrefixLayer() = default;

    /// @brief Copy constructor
    ChecksumPrefixLayer(const ChecksumPrefixLayer&) = default;

    /// @brief Move constructor
    ChecksumPrefixLayer(ChecksumPrefixLayer&&) = default;

    /// @brief Destructor.
    ~ChecksumPrefixLayer() noexcept = default;

    /// @brief Copy assignment
    ChecksumPrefixLayer& operator=(const ChecksumPrefixLayer&) = default;

    /// @brief Move assignment
    ChecksumPrefixLayer& operator=(ChecksumPrefixLayer&&) = default;

    /// @brief Deserialise message from the input data sequence.
    /// @details First, reads the expected checksum value field, then
    ///     executes the read() member function of the next layer.
    ///     If the call returns comms::ErrorStatus::Success, it calculated the
    ///     checksum of the read data and
    ///     compares it to the calculated. If checksums match,
    ///     comms::ErrorStatus::Success is returned, otherwise
    ///     function returns comms::ErrorStatus::ProtocolError.
    /// @tparam TMsgPtr Type of smart pointer that holds message object.
    /// @tparam TIter Type of iterator used for reading.
    /// @param[in, out] msgPtr Reference to smart pointer that already holds or
    ///     will hold allocated message object
    /// @param[in, out] iter Input iterator used for reading.
    /// @param[in] size Size of the data in the sequence
    /// @param[out] missingSize If not nullptr and return value is
    ///     comms::ErrorStatus::NotEnoughData it will contain
    ///     minimal missing data length required for the successful
    ///     read attempt.
    /// @return Status of the read operation.
    /// @pre Iterator must be "random access" one.
    /// @pre Iterator must be valid and can be dereferenced and incremented at
    ///      least "size" times;
    /// @post The iterator will be advanced by the number of bytes was actually
    ///       read. In case of an error, distance between original position and
    ///       advanced will pinpoint the location of the error.
    /// @post missingSize output value is updated if and only if function
    ///       returns comms::ErrorStatus::NotEnoughData.
    template <typename TMsgPtr, typename TIter>
    ErrorStatus read(
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize = nullptr)
    {
        using IterType = typename std::decay<decltype(iter)>::type;
        static_assert(std::is_same<typename std::iterator_traits<IterType>::iterator_category, std::random_access_iterator_tag>::value,
            "The read operation is expected to use random access iterator");

        Field field;
        return
            readInternal(
                field,
                msgPtr,
                iter,
                size,
                missingSize,
                Base::createNextLayerReader());
    }

    /// @brief Deserialise message from the input data sequence while caching
    ///     the read transport information fields.
    /// @details Very similar to read() member function, but adds "allFields"
    ///     parameter to store read transport information fields.
    /// @tparam TIdx Index of the message ID field in TAllFields tuple.
    /// @tparam TAllFields std::tuple of all the transport fields, must be
    ///     @ref AllFields type defined in the last layer class that defines
    ///     protocol stack.
    /// @tparam TMsgPtr Type of smart pointer that holds message object.
    /// @tparam TIter Type of iterator used for reading.
    /// @param[out] allFields Reference to the std::tuple object that wraps all
    ///     transport fields (@ref AllFields type of the last protocol layer class).
    /// @param[in] msgPtr Reference to the smart pointer holding message object.
    /// @param[in, out] iter Iterator used for reading.
    /// @param[in] size Number of bytes available for reading.
    /// @param[out] missingSize If not nullptr and return value is
    ///             comms::ErrorStatus::NotEnoughData it will contain
    ///             minimal missing data length required for the successful
    ///             read attempt.
    /// @return Status of the operation.
    template <std::size_t TIdx, typename TAllFields, typename TMsgPtr, typename TIter>
    ErrorStatus readFieldsCached(
        TAllFields& allFields,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize = nullptr)
    {
        using IterType = typename std::decay<decltype(iter)>::type;
        static_assert(std::is_same<typename std::iterator_traits<IterType>::iterator_category, std::random_access_iterator_tag>::value,
            "The read operation is expected to use random access iterator");

        auto& field = Base::template getField<TIdx>(allFields);

        return
            readInternal(
                field,
                msgPtr,
                iter,
                size,
                missingSize,
                Base::template createNextLayerCachedFieldsReader<TIdx>(allFields));
    }

    /// @brief Serialise message into the output data sequence.
    /// @details First, reserves the appropriate number of bytes in the
    ///     output buffer which are supposed to contain valid checksum
    ///     value, then executes the write() member function of the next layer.
    ///     If the call returns comms::ErrorStatus::Success and it is possible
    ///     to re-read what has been written (random access iterator is used
    ///     for writing), the real checksum value is calculated and updated in the
    ///     previously reserved area. In case non-random access iterator
    ///     type is used for writing (for example std::back_insert_iterator), then
    ///     this function returns
    ///     comms::ErrorStatus::UpdateRequired to indicate that call to
    ///     update() with random access iterator is required in order to be
    ///     able to update written checksum information.
    /// @tparam TMsg Type of message object.
    /// @tparam TIter Type of iterator used for writing.
    /// @param[in] msg Reference to message object
    /// @param[in, out] iter Output iterator.
    /// @param[in] size Max number of bytes that can be written.
    /// @return Status of the write operation.
    /// @pre Iterator must be valid and can be dereferenced and incremented at
    ///      least "size" times;
    /// @post The iterator will be advanced by the number of bytes was actually
    ///       written. In case of an error, distance between original position
    ///       and advanced will pinpoint the location of the error.
    template <typename TMsg, typename TIter>
    ErrorStatus write(const TMsg& msg, TIter& iter, std::size_t size) const
    {
        using IterType = typename std::decay<decltype(iter)>::type;
        using Tag = typename std::iterator_traits<IterType>::iterator_category;

        Field field;
        return writeInternal(field, msg, iter, size, Base::createNextLayerWriter(), Tag());
    }

    /// @brief Serialise message into output data sequence while caching the written transport
    ///     information fields.
    /// @details Very similar to write() member function, but adds "allFields"
    ///     parameter to store raw data of the message.
    /// @tparam TIdx Index of the data field in TAllFields, expected to be last
    ///     element in the tuple.
    /// @tparam TAllFields std::tuple of all the transport fields, must be
    ///     @ref AllFields type defined in the last layer class that defines
    ///     protocol stack.
    /// @tparam TMsg Type of message object.
    /// @tparam TIter Type of iterator used for writing.
    /// @param[out] allFields Reference to the std::tuple object that wraps all
    ///     transport fields (@ref AllFields type of the last protocol layer class).
    /// @param[in] msg Reference to the message object that is being written,
    /// @param[in, out] iter Iterator used for writing.
    /// @param[in] size Max number of bytes that can be written.
    /// @return Status of the write operation.
    template <std::size_t TIdx, typename TAllFields, typename TMsg, typename TIter>
    ErrorStatus writeFieldsCached(
        TAllFields& allFields,
        const TMsg& msg,
        TIter& iter,
        std::size_t size) const
    {
        using IterType = typename std::decay<decltype(iter)>::type;
        using Tag = typename std::iterator_traits<IterType>::iterator_category;
        auto& field = Base::template getField<TIdx>(allFields);
        field.value() = 0;
        return
            writeInternal(
                field,
                msg,
                iter,
                size,
                Base::template createNextLayerCachedFieldsWriter<TIdx>(allFields),
                Tag());
    }

    /// @brief Update written dummy checksum with proper value.
    /// @details Should be called when write() returns comms::ErrorStatus::UpdateRequired.
    /// @tparam TIter Type of iterator used for updating.
    /// @param[in, out] iter Any random access iterator.
    /// @param[in] size Number of bytes that have been written using write().
    /// @return Status of the update operation.
    template <typename TIter>
    comms::ErrorStatus update(TIter& iter, std::size_t size) const
    {
        Field field;
        return updateInternal(field, iter, size, Base::createNextLayerUpdater());
    }

    /// @brief Update written dummy checksum with proper value while caching the written transport
    ///     information fields.
    /// @details Very similar to update() member function, but adds "allFields"
    ///     parameter to store raw data of the message.
    /// @tparam TIdx Index of the data field in TAllFields, expected to be last
    ///     element in the tuple.
    /// @tparam TAllFields std::tuple of all the transport fields, must be
    ///     @ref AllFields type defined in the last layer class that defines
    ///     protocol stack.
    /// @tparam TIter Type of iterator used for updating.
    /// @param[out] allFields Reference to the std::tuple object that wraps all
    ///     transport fields (@ref AllFields type of the last protocol layer class).
    /// @param[in, out] iter Any random access iterator.
    /// @param[in] size Max number of bytes that can be written.
    /// @return Status of the update operation.
    template <std::size_t TIdx, typename TAllFields, typename TIter>
    ErrorStatus updateFieldsCached(
        TAllFields& allFields,
        TIter& iter,
        std::size_t size) const
    {
        auto& field = Base::template getField<TIdx>(allFields);
        return
            updateInternal(
                field,
                iter,
                size,
                Base::template createNextLayerCachedFieldsUpdater<TIdx>(allFields));
    }

private:
    static_assert(comms::field::isIntValue<Field>(),
        "The checksum field is expected to be of IntValue type");

    static_assert(Field::minLength() == Field::maxLength(),
        "The checksum field is expected to be of fixed length");

    struct VerifyBeforeReadTag {};
    struct VerifyAfterReadTag {};

    using VerifyTag = typename
        std::conditional<
            ParsedOptions::HasVerifyBeforeRead,
            VerifyBeforeReadTag,
            VerifyAfterReadTag
        >::type;

    template <typename TMsgPtr, typename TIter, typename TReader>
    ErrorStatus readInternal(
        Field& field,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize,
        TReader&& nextLayerReader)
    {
        if (size < Field::minLength()) {
            return ErrorStatus::NotEnoughData;
        }

        auto checksumEs = field.read(iter, Field::minLength());
        if (checksumEs == ErrorStatus::NotEnoughData) {
            Base::updateMissingSize(field, size, missingSize);
        }

        if (checksumEs != ErrorStatus::Success) {
            return checksumEs;
        }

        return readInternal(field, msgPtr, iter, size - field.length(), missingSize, std::forward<TReader>(nextLayerReader), VerifyTag());
    }

    template <typename TMsgPtr, typename TIter, typename TReader>
    ErrorStatus verifyRead(
        Field& field,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize,
        TReader&& nextLayerReader)
    {
        auto fromIter = iter;

        auto checksum = TCalc()(fromIter, size);
        auto expectedValue = field.value();

        if (expectedValue != static_cast<decltype(expectedValue)>(checksum)) {
            msgPtr.reset();
            return ErrorStatus::ProtocolError;
        }

        return nextLayerReader.read(msgPtr, iter, size, missingSize);
    }

    template <typename TMsgPtr, typename TIter, typename TReader>
    ErrorStatus readVerify(
        Field& field,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize,
        TReader&& nextLayerReader)
    {
        auto fromIter = iter;

        auto es = nextLayerReader.read(msgPtr, iter, size, missingSize);
        if ((es == ErrorStatus::NotEnoughData) ||
            (es == ErrorStatus::ProtocolError)) {
            return es;
        }

        auto len = static_cast<std::size_t>(std::distance(fromIter, iter));
        auto checksum = TCalc()(fromIter, len);
        auto expectedValue = field.value();

        if (expectedValue != static_cast<decltype(expectedValue)>(checksum)) {
            msgPtr.reset();
            return ErrorStatus::ProtocolError;
        }

        return es;
    }

    template <typename TMsgPtr, typename TIter, typename TReader>
    ErrorStatus readInternal(
        Field& field,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize,
        TReader&& nextLayerReader,
        VerifyBeforeReadTag)
    {
        return verifyRead(field, msgPtr, iter, size, missingSize, std::forward<TReader>(nextLayerReader));
    }

    template <typename TMsgPtr, typename TIter, typename TReader>
    ErrorStatus readInternal(
        Field& field,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize,
        TReader&& nextLayerReader,
        VerifyAfterReadTag)
    {
        return readVerify(field, msgPtr, iter, size, missingSize, std::forward<TReader>(nextLayerReader));
    }

    template <typename TMsg, typename TIter, typename TWriter>
    ErrorStatus writeInternalRandomAccess(
        Field& field,
        const TMsg& msg,
        TIter& iter,
        std::size_t size,
        TWriter&& nextLayerWriter) const
    {
        auto checksumIter = iter;
        auto es = field.write(iter, size);
        if (es != comms::ErrorStatus::Success) {
            return es;
        }

        auto checksumLen =
            static_cast<std::size_t>(std::distance(checksumIter, iter));

        auto fromIter = iter;
        es = nextLayerWriter.write(msg, iter, size - checksumLen);
        if (es != comms::ErrorStatus::Success) {
            return es;
        }

        GASSERT(fromIter <= iter);
        auto len = static_cast<std::size_t>(std::distance(fromIter, iter));

        using FieldValueType = typename Field::ValueType;
        auto checksum = TCalc()(fromIter, len);
        field.value() = static_cast<FieldValueType>(checksum);

        auto checksumEs = field.write(checksumIter, checksumLen);
        static_cast<void>(checksumEs);
        GASSERT(checksumEs == comms::ErrorStatus::Success);
        return es;
    }

    template <typename TMsg, typename TIter, typename TWriter>
    ErrorStatus writeInternalOutput(
        Field& field,
        const TMsg& msg,
        TIter& iter,
        std::size_t size,
        TWriter&& nextLayerWriter) const
    {
        auto es = field.write(iter, size);
        if (es != comms::ErrorStatus::Success) {
            return es;
        }

        es = nextLayerWriter.write(msg, iter, size - Field::maxLength());
        if (es != comms::ErrorStatus::Success) {
            return es;
        }

        return comms::ErrorStatus::UpdateRequired;
    }

    template <typename TMsg, typename TIter, typename TWriter>
    ErrorStatus writeInternal(
        Field& field,
        const TMsg& msg,
        TIter& iter,
        std::size_t size,
        TWriter&& nextLayerWriter,
        std::random_access_iterator_tag) const
    {
        return writeInternalRandomAccess(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter));
    }

    template <typename TMsg, typename TIter, typename TWriter>
    ErrorStatus writeInternal(
        Field& field,
        const TMsg& msg,
        TIter& iter,
        std::size_t size,
        TWriter&& nextLayerWriter,
        std::output_iterator_tag) const
    {
        return writeInternalOutput(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter));
    }

    template <typename TIter, typename TUpdater>
    ErrorStatus updateInternal(
        Field& field,
        TIter& iter,
        std::size_t size,
        TUpdater&& nextLayerUpdater) const
    {
        auto checksumIter = iter;
        std::advance(iter, Field::maxLength());

        auto fromIter = iter;
        auto es = nextLayerUpdater.update(iter, size - Field::maxLength());
        if (es != comms::ErrorStatus::Success) {
            return es;
        }

        GASSERT(fromIter <= iter);
        auto len = static_cast<std::size_t>(std::distance(fromIter, iter));
        GASSERT(len == (size - Field::maxLength()));
        using FieldValueType = typename Field::ValueType;
        field.value() = static_cast<FieldValueType>(TCalc()(fromIter, len));
        es = field.write(checksumIter, Field::maxLength());
        return es;
    }
};

}  // namespace protocol

}  // namespace comms



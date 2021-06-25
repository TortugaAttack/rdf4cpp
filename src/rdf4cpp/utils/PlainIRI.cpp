//
// Created by Lixi Alié Conrads on 5/31/21.
//

#include "PlainIRI.h"
#include "error/InvalidIRIError.h"
#include "sec/Err.h"
#include "sec/Ok.h"
#include "sec/Result.h"
#include <algorithm>
#include <ctype.h>
#include <utility>
#include <vector>

/*
 *
 * What we do is to check each if each var is valid with mathematical ops or built in ops and find the end char
 *
 */
rdf4cpp::utils::sec::Result<rdf4cpp::utils::PlainIRI, rdf4cpp::utils::error::InvalidIRIError> rdf4cpp::utils::PlainIRI::create(std::string iri_string) {
    //TODO validate IRI RFC3986
    // : allowed are < ALPHA / DIGIT / "+" / "-" / "." > First one is ALPHA
    // // -> up to / ? # or end
    // -> /
    // -> ? [&] #
    // ->

    //TODO use UTF8String instead of string.
    if(!isalnum(iri_string[0])){
        auto error = rdf4cpp::utils::error::InvalidIRIError("Scheme doesn't start with ALPHANUMERIC character.");
        return rdf4cpp::utils::sec::Err<rdf4cpp::utils::PlainIRI, rdf4cpp::utils::error::InvalidIRIError>(error);
    }
    auto allowed = std::vector<std::string>();
    for (size_t i=1;i<iri_string.length();i++ ){
        //TODO manually do UTF-8 because WHAT THE FUCK C++ in 2021
        //
    }
    PlainIRI iri = PlainIRI(std::move(iri_string));
    return rdf4cpp::utils::sec::Ok<rdf4cpp::utils::PlainIRI, rdf4cpp::utils::error::InvalidIRIError>(iri);
}

std::string rdf4cpp::utils::PlainIRI::get_iri() const {
    return this->iri;
}

rdf4cpp::utils::PlainIRI::PlainIRI(std::string iri_string){
    this->iri=std::move(iri_string);
}

bool rdf4cpp::utils::PlainIRI::is_valid(std::string &iri_string) {
    if(rdf4cpp::utils::PlainIRI::create(iri_string).is_ok()){
        return true;
    }
    return false;
}

bool rdf4cpp::utils::PlainIRI::is_uri() const {
    for(char c : this->iri){
        if(!isascii(c)){
            return false;
        }
    }
    return true;
};
